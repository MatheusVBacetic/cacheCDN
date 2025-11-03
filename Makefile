# ---------------------------------------
# Makefile - Projeto Prático 2 (OpenMP)
# ---------------------------------------

# Compilador e flags
CC = gcc
CFLAGS = -O2 -Wall -fopenmp

# Arquivos de código
HASH = hash_table.c
HEADERS = hash_table.h

# Executáveis
SEQ = seq
PAR_CRIT = par_critical
PAR_ATOM = par_atomic

# ---------------------------------------
# Regras de compilação
# ---------------------------------------

all: $(SEQ) $(PAR_CRIT) $(PAR_ATOM)

# Versão sequencial
$(SEQ): analyzer_seq.c $(HASH) $(HEADERS)
	$(CC) $(CFLAGS) analyzer_seq.c $(HASH) -o $(SEQ)

# Versão paralela com critical
$(PAR_CRIT): analyzer_par_critical.c $(HASH) $(HEADERS)
	$(CC) $(CFLAGS) analyzer_par_critical.c $(HASH) -o $(PAR_CRIT)

# Versão paralela com atomic
$(PAR_ATOM): analyzer_par_atomic.c $(HASH) $(HEADERS)
	$(CC) $(CFLAGS) analyzer_par_atomic.c $(HASH) -o $(PAR_ATOM)

# ---------------------------------------
# Limpeza
# ---------------------------------------

clean:
	rm -f $(SEQ) $(PAR_CRIT) $(PAR_ATOM) *.o results.csv

# ---------------------------------------
# Testes rápidos
# ---------------------------------------

run-seq:
	./$(SEQ) manifest.txt log_distribuido.txt

run-critical:
	OMP_NUM_THREADS=8 ./$(PAR_CRIT) manifest.txt log_distribuido.txt

run-atomic:
	OMP_NUM_THREADS=8 ./$(PAR_ATOM) manifest.txt log_distribuido.txt
