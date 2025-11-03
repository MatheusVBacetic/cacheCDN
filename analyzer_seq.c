#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "hash_table.h"

#define MAX_LINE 8192

static void strip_bom(char *s) {
    if (!s) return;
    unsigned char *u = (unsigned char*)s;
    if (u[0] == 0xEF && u[1] == 0xBB && u[2] == 0xBF) {
        // shift left 3 bytes
        memmove(s, s + 3, strlen(s + 3) + 1);
    }
}

static void chomp(char *s) {
    if (!s) return;
    s[strcspn(s, "\r\n")] = '\0';
}

static void trim(char *s) {
    if (!s) return;
    // left trim
    size_t i = 0;
    while (s[i] == ' ' || s[i] == '\t') i++;
    if (i) memmove(s, s + i, strlen(s + i) + 1);
    // right trim
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t')) {
        s[--len] = '\0';
    }
}

// Se a linha tiver espaços (ex.: "GET http://... 200"), fica só o primeiro token que deve ser a URL.
// Se o gerador já produz só a URL, isso não altera nada.
static void normalize_line(char *s) {
    if (!s) return;
    strip_bom(s);
    chomp(s);
    trim(s);
    // pega somente o primeiro token (até espaço/tab)
    char *p = s;
    while (*p && *p != ' ' && *p != '\t') p++;
    *p = '\0';
}

static size_t count_lines(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Erro abrindo '%s': %s\n", path, strerror(errno));
        return 0;
    }
    size_t n = 0;
    char buf[MAX_LINE];
    while (fgets(buf, sizeof(buf), fp)) {
        normalize_line(buf);
        if (buf[0] == '\0') continue;
        n++;
    }
    fclose(fp);
    return n;
}

static int load_manifest(HashTable *ht, const char *manifest_path) {
    FILE *fp = fopen(manifest_path, "r");
    if (!fp) {
        fprintf(stderr, "Erro abrindo manifest '%s': %s\n", manifest_path, strerror(errno));
        return 0;
    }
    char line[MAX_LINE];
    size_t added = 0;
    while (fgets(line, sizeof(line), fp)) {
        normalize_line(line);
        if (line[0] == '\0') continue;
        ht_put(ht, line);
        added++;
    }
    fclose(fp);
    fprintf(stdout, "[SEQ] Manifest carregado: %zu URLs\n", added);
    return 1;
}

static int process_log(HashTable *ht, const char *log_path) {
    FILE *fp = fopen(log_path, "r");
    if (!fp) {
        fprintf(stderr, "Erro abrindo access_log '%s': %s\n", log_path, strerror(errno));
        return 0;
    }
    char line[MAX_LINE];
    long processed = 0, misses = 0, hits = 0;

    while (fgets(line, sizeof(line), fp)) {
        normalize_line(line);
        if (line[0] == '\0') continue;

        CacheNode *node = ht_get(ht, line);
        if (node) {
            node->hit_count++;
            hits++;
        } else {
            misses++;
        }
        processed++;
        if (processed % 1000000L == 0) {
            fprintf(stdout, "[SEQ] Processadas %ld linhas... (hits=%ld, misses=%ld)\n",
                    processed, hits, misses);
        }
    }
    fclose(fp);
    fprintf(stdout, "[SEQ] Total processado: %ld | hits=%ld | misses=%ld (%.2f%%)\n",
            processed, hits, misses,
            processed ? (100.0 * misses / processed) : 0.0);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <manifest.txt> <access_log.txt>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *manifest_path = argv[1];
    const char *log_path      = argv[2];

    size_t urls = count_lines(manifest_path);
    if (urls == 0) {
        fprintf(stderr, "Manifest vazio ou inacessível.\n");
        return EXIT_FAILURE;
    }
    size_t ht_size = urls * 2 + 1;

    fprintf(stdout, "[SEQ] Criando HashTable com size=%zu (urls=%zu)\n", ht_size, urls);
    HashTable *ht = ht_create(ht_size);
    if (!ht) {
        fprintf(stderr, "Falha ao criar HashTable.\n");
        return EXIT_FAILURE;
    }

    if (!load_manifest(ht, manifest_path)) {
        ht_destroy(ht);
        return EXIT_FAILURE;
    }

    fprintf(stdout, "[SEQ] Processando log: %s\n", log_path);
    if (!process_log(ht, log_path)) {
        ht_destroy(ht);
        return EXIT_FAILURE;
    }

    fprintf(stdout, "[SEQ] Salvando resultados em results.csv\n");
    ht_save_results(ht, "results.csv");

    ht_destroy(ht);
    fprintf(stdout, "[SEQ] Concluído com sucesso.\n");
    return EXIT_SUCCESS;
}
