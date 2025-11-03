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
# Execuções - Resultados separados
# ---------------------------------------

# Sequencial - logs diferentes
run-seq-distribuido:
	./$(SEQ) manifest.txt log_distribuido.txt results_distribuido.csv

run-seq-concorrente:
	./$(SEQ) manifest.txt log_concorrente.txt results_concorrente.csv

# Paralela (critical)
run-critical-distribuido:
	OMP_NUM_THREADS=8 ./$(PAR_CRIT) manifest.txt log_distribuido.txt results_distribuido_critical.csv

run-critical-concorrente:
	OMP_NUM_THREADS=8 ./$(PAR_CRIT) manifest.txt log_concorrente.txt results_concorrente_critical.csv

# Paralela (atomic)
run-atomic-distribuido:
	OMP_NUM_THREADS=8 ./$(PAR_ATOM) manifest.txt log_distribuido.txt results_distribuido_atomic.csv

run-atomic-concorrente:
	OMP_NUM_THREADS=8 ./$(PAR_ATOM) manifest.txt log_concorrente.txt results_concorrente_atomic.csv

# ---------------------------------------
# Limpeza
# ---------------------------------------

clean:
	rm -f $(SEQ) $(PAR_CRIT) $(PAR_ATOM) *.o results*.csv

# ---------------------------------------
# Ajuda
# ---------------------------------------

help:
	@echo "Comandos disponíveis:"
	@echo "  make                  -> Compila todos os executáveis"
	@echo "  make run-seq-distribuido     -> Executa versão sequencial (baixa contenção)"
	@echo "  make run-seq-concorrente     -> Executa versão sequencial (alta contenção)"
	@echo "  make run-critical-distribuido -> Executa paralela critical (baixa contenção)"
	@echo "  make run-critical-concorrente -> Executa paralela critical (alta contenção)"
	@echo "  make run-atomic-distribuido   -> Executa paralela atomic (baixa contenção)"
	@echo "  make run-atomic-concorrente   -> Executa paralela atomic (alta contenção)"
	@echo "  make clean             -> Remove executáveis e CSVs gerados"
