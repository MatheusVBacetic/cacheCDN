// analyzer_seq.c (parser corrigido para Apache log)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "hash_table.h"

#define MAX_LINE 8192

static void strip_bom(char *s){ if(!s) return; unsigned char *u=(unsigned char*)s; if(u[0]==0xEF&&u[1]==0xBB&&u[2]==0xBF){ memmove(s,s+3,strlen(s+3)+1);} }
static void chomp(char *s){ if(!s) return; s[strcspn(s,"\r\n")]='\0'; }
static void trim(char *s){
    if(!s) return;
    size_t i=0; while(s[i]==' '||s[i]=='\t') i++;
    if(i) memmove(s,s+i,strlen(s+i)+1);
    size_t len=strlen(s); while(len>0&&(s[len-1]==' '||s[len-1]=='\t')) s[--len]='\0';
}

/* Extrai a URL do access log no formato Apache:
   ... "GET /caminho/arquivo.ext HTTP/1.1" ...
   Regras:
   1) Se houver aspas, pega o trecho entre aspas, pula o método e captura o token que começa com '/'.
   2) Se não houver aspas, se a linha começar com '/', pega o primeiro token até espaço/tab.
   3) Se houver http(s)://, usa o token da URL completa (até espaço/tab).
   4) Caso contrário, usa o primeiro token (fallback).
*/
static void extract_url(char *s){
    strip_bom(s); chomp(s); trim(s);

    // 1) Tentar formato entre aspas: "GET /path HTTP/1.1"
    char *first_quote = strchr(s, '"');
    if (first_quote) {
        char *q = first_quote + 1;              // início do método
        // pular o método (até espaço)
        while (*q && *q != ' ') q++;
        if (*q == ' ') q++;                     // agora q deve estar no começo do caminho
        // se não começar com '/', procurar primeiro '/' subsequente
        while (*q && *q != '/' && *q != '"') q++;
        if (*q == '/') {
            // copiar o caminho para o início e cortar no próximo espaço ou aspas
            char *end = q;
            while (*end && *end != ' ' && *end != '"') end++;
            *end = '\0';
            if (q != s) memmove(s, q, strlen(q) + 1);
            return;
        }
        // se chegou aqui, não achou '/', cai para outras heurísticas
    }

    // 2) Linha já é um caminho
    if (s[0] == '/') {
        // corta no primeiro espaço/tab
        char *p = s; while (*p && *p != ' ' && *p != '\t') p++; *p = '\0';
        return;
    }

    // 3) URL completa
    char *start = strstr(s, "http://");
    if (!start) start = strstr(s, "https://");
    if (start) {
        if (start != s) memmove(s, start, strlen(start) + 1);
        char *p = s; while (*p && *p != ' ' && *p != '\t') p++; *p = '\0';
        return;
    }

    // 4) Fallback: primeiro token
    char *p = s; while (*p && *p != ' ' && *p != '\t') p++; *p = '\0';
}

static size_t count_urls(const char *path){
    FILE *fp=fopen(path,"r"); if(!fp){ fprintf(stderr,"Erro '%s': %s\n", path, strerror(errno)); return 0; }
    size_t n=0; char buf[MAX_LINE];
    while(fgets(buf,sizeof(buf),fp)){ extract_url(buf); if(buf[0]) n++; }
    fclose(fp); return n;
}

static int load_manifest(HashTable *ht, const char *manifest){
    FILE *fp=fopen(manifest,"r"); if(!fp){ fprintf(stderr,"Erro manifest '%s': %s\n", manifest, strerror(errno)); return 0; }
    char line[MAX_LINE]; size_t added=0;
    while(fgets(line,sizeof(line),fp)){
        extract_url(line);
        if(!line[0]) continue;
        ht_put(ht, line);
        added++;
    }
    fclose(fp);
    printf("[SEQ] Manifest carregado: %zu URLs\n", added);
    return 1;
}

static void to_visible(const char *src, char *dst, size_t cap){
    size_t j=0;
    for(size_t i=0; src[i] && j+4<cap; i++){
        unsigned char c=src[i];
        if(c=='\n'){ dst[j++]='\\'; dst[j++]='n'; }
        else if(c=='\r'){ dst[j++]='\\'; dst[j++]='r'; }
        else if(c=='\t'){ dst[j++]='\\'; dst[j++]='t'; }
        else if(c<32 || c==127){ j+=snprintf(dst+j, cap-j, "\\x%02X", c); }
        else dst[j++]=c;
    }
    dst[j]='\0';
}

static int process_log(HashTable *ht, const char *log){
    FILE *fp=fopen(log,"r"); if(!fp){ fprintf(stderr,"Erro log '%s': %s\n", log, strerror(errno)); return 0; }
    char line[MAX_LINE]; long processed=0, hits=0, misses=0; int shown=0;

    while(fgets(line,sizeof(line),fp)){
        extract_url(line);
        if(!line[0]) continue;

        CacheNode *node=ht_get(ht, line);
        if(node){ node->hit_count++; hits++; }
        else{
            misses++;
            if(shown<5){
                char vis[1024]; to_visible(line, vis, sizeof(vis));
                printf("[MISS exemplo] '%s' (len=%zu)\n", vis, strlen(line));
                shown++;
            }
        }
        processed++;
        if(processed % 1000000L == 0)
            printf("[SEQ] %ld linhas... (hits=%ld, misses=%ld)\n", processed, hits, misses);
    }
    fclose(fp);
    printf("[SEQ] Total=%ld | hits=%ld | misses=%ld (%.2f%% misses)\n",
           processed, hits, misses, processed? (100.0*misses/processed):0.0);
    return 1;
}

int main(int argc, char **argv){
    if(argc < 3 || argc > 4){
        fprintf(stderr,"Uso: %s <manifest.txt> <access_log.txt> [results.csv]\n", argv[0]);
        return 1;
    }
    const char *manifest = argv[1];
    const char *log = argv[2];
    const char *out = (argc == 4) ? argv[3] : "results.csv"; // default

    size_t urls = count_urls(manifest);
    if(urls == 0){ fprintf(stderr,"Manifest vazio/inválido.\n"); return 1; }

    size_t ht_size = urls*2+1;
    printf("[SEQ] HashTable size=%zu (urls=%zu)\n", ht_size, urls);

    HashTable *ht = ht_create(ht_size);
    if(!ht){ fprintf(stderr,"Falha ht_create\n"); return 1; }

    if(!load_manifest(ht, manifest)){ ht_destroy(ht); return 1; }

    printf("[SEQ] Processando: %s\n", log);
    if(!process_log(ht, log)){ ht_destroy(ht); return 1; }

    printf("[SEQ] Salvando resultados em %s\n", out);
    ht_save_results(ht, out);

    ht_destroy(ht);
    printf("[SEQ] OK.\n");
    return 0;
}

