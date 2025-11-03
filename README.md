# Projeto Prático 2 — Computação Paralela (CDN Analyzer)

Trabalho desenvolvido para a disciplina **Computação Paralela**, ministrada pelo **Prof. Dr. Jean M. Laine** — Universidade Presbiteriana Mackenzie.

---

## 🎯 Objetivo

Simular o mecanismo de análise de uma **Content Delivery Network (CDN)**, que identifica conteúdos mais acessados ("hot content") e menos acessados ("cold content").
O sistema processa milhões de acessos a URLs a partir de logs, atualizando uma tabela hash em memória e comparando o desempenho entre diferentes estratégias de paralelismo com OpenMP.

---

## 📝 Estrutura do Projeto

```
📁 Projeto-Pratico-2/
├── analyzer_seq.c              # Versão sequencial
├── analyzer_par_critical.c     # Versão paralela (granularidade grossa)
├── analyzer_par_atomic.c       # Versão paralela (granularidade fina)
├── hash_table.c / hash_table.h # Implementação da tabela hash
├── Makefile                    # Compilação e execução automatizada
├── manifest.txt                # URLs únicas da CDN
├── log_distribuido.txt         # Log com acessos uniformes (baixa contenção)
├── log_concorrente.txt         # Log com acessos concentrados (alta contenção)
├── gabarito_distribuido.csv    # Resultados esperados (baixa contenção)
├── gabarito_concorrente.csv    # Resultados esperados (alta contenção)
└── relatorio.pdf               # Relatório final (entrega acadêmica)
```

---

## ⚙️ Compilação

Para compilar todas as versões:

```bash
make
```

---

## 🚀 Execução

### 🔹 Versão Sequencial

Baixa contenção:

```bash
make run-seq-distribuido
```

Alta contenção:

```bash
make run-seq-concorrente
```

### 🔹 Versão Paralela (Critical)

Baixa contenção:

```bash
OMP_NUM_THREADS=8 ./par_critical manifest.txt log_distribuido.txt results_distribuido_critical.csv
```

Alta contenção:

```bash
OMP_NUM_THREADS=8 ./par_critical manifest.txt log_concorrente.txt results_concorrente_critical.csv
```

### 🔹 Versão Paralela (Atomic)

Baixa contenção:

```bash
OMP_NUM_THREADS=8 ./par_atomic manifest.txt log_distribuido.txt results_distribuido_atomic.csv
```

Alta contenção:

```bash
OMP_NUM_THREADS=8 ./par_atomic manifest.txt log_concorrente.txt results_concorrente_atomic.csv
```

---

## ⏱️ Medidas de Desempenho

Use o comando `time` para medir o tempo de execução de cada versão:

```bash
/usr/bin/time -f "%E" ./par_atomic manifest.txt log_distribuido.txt results.csv
```

Calcule:

* **Speedup:** `tempo_seq / tempo_paralelo`
* **Eficiência:** `speedup / número_de_threads`

---

## 🧠 Conceitos Envolvidos

* **Granularidade Grosseira:** `#pragma omp critical`
  → Apenas uma thread por vez dentro da região crítica.
  → Segura, mas causa gargalo.

* **Granularidade Fina:** `#pragma omp atomic`
  → Protege apenas a variável `hit_count`.
  → Desempenho muito melhor sob alta carga.

---

## 🏁 Conclusão

O projeto demonstra, na prática, o impacto da **contenção de locks** em sistemas paralelos e como a escolha correta da **granularidade de sincronização** determina a **eficiência** de uma aplicação real, como o motor de replicacão de cache de uma CDN.

---

## 👨‍💻 Autores

* **Matheus Veiga Bacetic Joaquim**
* **João Vitor Rocha Miranda**

---

## 🏫 Universidade Presbiteriana Mackenzie

**Faculdade de Computação e Informática (FCI)**
Disciplina: Computação Paralela
Professor: **Dr. Jean M. Laine**
