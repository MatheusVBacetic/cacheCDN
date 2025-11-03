// analyzer_par_critical.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <omp.h>
#include "hash_table.h"

#define MAX_LINE     8192
#define BATCH_LINES  200000   // processa o log em lotes para economizar memória

static void strip_bom(char *s){ if(!s) return; unsigned char *u=(unsigned char*)s; if(u[0]==0xEF&&u[1]==0xBB&&u[2]==0xBF){ memmove(s,s+3,strlen(s+3)+1);} }
static void chomp(char *s){ if(!s) return; s[strcspn(s,"\r\n")]='\0'; }
static void trim(char *s){ if(!s) return; size_t i=0; while(s[i]==' '||s[i]=='\t') i++; if(i) memmove(s,s+i,strlen(s+i)+1); size_t n=strlen(s); while(n>0&&(s[n-1]==' '||s[n-1]=='\t')) s[--n]='\0'; }

/* Extrai o caminho do formato Apache: "GET /path HTTP/1.1" */
static void extract_url(char *s){
    strip_bom(s); chomp(s); trim(s);
    char *first_quote = strchr(s, '"');
    if (first_quote) {
        char *q = first_quote + 1;
        while (*q && *q != ' ') q++;
        if (*q == ' ') q++;
        while (*q && *q != '/' && *q != '"') q++;
        if (*q == '/') {
            char *end = q;
            while (*end && *end != ' ' && *end != '"') end++;
            *end = '\0';
            if (q != s) memmove(s, q, strlen(q) + 1);
            return;
        }
    }
    if (s[0] == '/') { char *p=s; while(*p && *p!=' ' && *p!='\t') p++; *p='\0'; return; }
    char *start = strstr(s,"http://"); if(!start) start=strstr(s,"https://");
    if (start){ if(start!=s) memmove(s,start,strlen(start)+1); char *p=s; while(*p && *p!=' ' && *p!='\t') p++; *p='\0'; return; }
    char *p=s; while(*p && *p!=' ' && *p!='\t') p++; *p='\0';
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
    while(fgets(line,sizeof(line),fp)){ extract_url(line); if(!line[0]) continue; ht_put(ht, line); added++; }
    fclose(fp); printf("[CRIT] Manifest carregado: %zu URLs\n", added); return 1;
}

int main(int argc, char **argv){
    if(argc < 3 || argc > 4){
        fprintf(stderr,"Uso: %s <manifest.txt> <access_log.txt> [results.csv]\n", argv[0]);
        return 1;
    }
    const char *manifest = argv[1];
    const char *log_path = argv[2];
    const char *out = (argc==4)? argv[3] : "results.csv";

    size_t urls = count_urls(manifest);
    if(urls==0){ fprintf(stderr,"Manifest vazio/inválido.\n"); return 1; }
    size_t ht_size = urls*2 + 1;
    printf("[CRIT] HashTable size=%zu (urls=%zu)\n", ht_size, urls);

    HashTable *ht = ht_create(ht_size);
    if(!ht){ fprintf(stderr,"Falha ht_create\n"); return 1; }
    if(!load_manifest(ht, manifest)){ ht_destroy(ht); return 1; }

    FILE *fp = fopen(log_path, "r");
    if(!fp){ fprintf(stderr,"Erro log '%s': %s\n", log_path, strerror(errno)); ht_destroy(ht); return 1; }

    char **batch = (char**)malloc(sizeof(char*)*BATCH_LINES);
    if(!batch){ perror("malloc batch"); fclose(fp); ht_destroy(ht); return 1; }

    char line[MAX_LINE];
    long processed_total=0;
    double t0 = omp_get_wtime();

    while(1){
        int n=0;
        while(n < BATCH_LINES && fgets(line,sizeof(line),fp)){
            extract_url(line);
            if(!line[0]) continue;
            batch[n] = strdup(line);
            if(!batch[n]){ perror("strdup"); fclose(fp); ht_destroy(ht); return 1; }
            n++;
        }
        if(n==0) break;

        #pragma omp parallel for schedule(static)
        for(int i=0;i<n;i++){
            CacheNode *node = ht_get(ht, batch[i]);
            if(node){
                #pragma omp critical
                {
                    node->hit_count++;
                }
            }
        }

        for(int i=0;i<n;i++) free(batch[i]);
        processed_total += n;

        if(processed_total % 1000000L == 0){
            printf("[CRIT] Processadas %ld linhas...\n", processed_total);
        }

        if(n < BATCH_LINES) break; // EOF
    }

    double t1 = omp_get_wtime();
    printf("[CRIT] Total processado: %ld linhas. Tempo: %.3fs\n", processed_total, t1-t0);

    printf("[CRIT] Salvando resultados em %s\n", out);
    ht_save_results(ht, out);

    free(batch);
    fclose(fp);
    ht_destroy(ht);
    printf("[CRIT] OK.\n");
    return 0;
}
