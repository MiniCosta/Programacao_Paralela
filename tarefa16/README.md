# Tarefa 16: Produto Matriz-Vetor Paralelo com MPI

## Descrição do Problema

Este programa implementa o **produto matriz-vetor y = A⋅x** utilizando MPI para paralelização eficiente. A implementação usa:

- **MPI_Scatter**: Distribui linhas da matriz A entre os processos
- **MPI_Bcast**: Distribui o vetor x completo para todos os processos
- **MPI_Gather**: Coleta os resultados parciais no processo 0

## Formulação Matemática

### Produto Matriz-Vetor

Dado:

- **Matriz A**: M×N (M linhas, N colunas)
- **Vetor x**: N elementos
- **Resultado y**: M elementos

Calcular: **y = A⋅x**

Onde cada elemento do resultado é:

```
y[i] = Σ(j=0 até N-1) A[i][j] * x[j]
```

### Complexidade Computacional

- **Operações**: 2×M×N (M×N multiplicações + M×N somas)
- **Complexidade temporal**: O(M×N)
- **Complexidade espacial**: O(M×N + M + N)

## Estratégia de Paralelização

### 1. Decomposição por Linhas

A matriz A é **dividida por linhas** entre os processos:

```
Matriz A (8×6 com 4 processos):

Processo 0: [A[0][*]]  [A[1][*]]  ← 2 linhas
Processo 1: [A[2][*]]  [A[3][*]]  ← 2 linhas
Processo 2: [A[4][*]]  [A[5][*]]  ← 2 linhas
Processo 3: [A[6][*]]  [A[7][*]]  ← 2 linhas
```

**Vantagens:**

- ✅ Balanceamento de carga perfeito
- ✅ Comunicação mínima
- ✅ Localidade de memória

### 2. Padrão de Comunicação

```
Fase 1: MPI_Bcast
┌─────────────────────────────────────┐
│ Processo 0: [x[0], x[1], ..., x[N]] │
└─────────────────┬───────────────────┘
                  │ Broadcast
        ┌─────────┼─────────┐
        ▼         ▼         ▼
    Proc 1    Proc 2    Proc 3
   [x completo] [x completo] [x completo]

Fase 2: MPI_Scatter
┌────────────────────────────────────────┐
│ Processo 0: [A completa M×N]           │
└─────────────────┬──────────────────────┘
                  │ Scatter por linhas
        ┌─────────┼─────────┐
        ▼         ▼         ▼
    Proc 1    Proc 2    Proc 3
   [A_local] [A_local] [A_local]
   (M/P×N)   (M/P×N)   (M/P×N)

Fase 3: Computação Local
Cada processo: y_local[i] = Σ A_local[i][j] * x[j]

Fase 4: MPI_Gather
    Proc 0    Proc 1    Proc 2    Proc 3
   [y_local] [y_local] [y_local] [y_local]
        │         │         │         │
        └─────────┼─────────┼─────────┘
                  ▼
        ┌─────────────────────────┐
        │ Processo 0: [y completo] │
        └─────────────────────────┘
```

## Implementação Detalhada

### Estrutura de Dados

```c
// Processo 0 (mestre)
double *A;              // Matriz completa M×N
double *y;              // Vetor resultado completo M

// Todos os processos
double *x;              // Vetor x completo N (após MPI_Bcast)
double *A_local;        // Submatriz local (M/P)×N
double *y_local;        // Resultado parcial M/P
```

### Algoritmo Principal

```c
1. Inicialização (Processo 0):
   - Alocar A, x, y
   - Inicializar A e x com valores aleatórios

2. MPI_Bcast:
   - Distribuir vetor x para todos os processos
   - Comunicação: O(N) elementos

3. MPI_Scatter:
   - Distribuir linhas de A entre processos
   - Cada processo recebe (M/P)×N elementos
   - Comunicação: O(M×N) total, O(M×N/P) por processo

4. Computação Local:
   for i = 0 to (M/P)-1:
       y_local[i] = 0
       for j = 0 to N-1:
           y_local[i] += A_local[i][j] * x[j]

5. MPI_Gather:
   - Coletar y_local de todos os processos
   - Comunicação: O(M) total, O(M/P) por processo
```

## Análise de Performance

### Tempo de Execução

**Tempo Total = T_comunicação + T_computação**

```
T_comunicação = T_bcast + T_scatter + T_gather
              ≈ α×log(P) + β×N +           // MPI_Bcast
                α×log(P) + β×(M×N/P) +     // MPI_Scatter
                α×log(P) + β×(M/P)         // MPI_Gather

T_computação = 2×M×N/P / (FLOPS_por_processo)
```

Onde:

- **α**: Latência de comunicação
- **β**: Inverso da largura de banda
- **P**: Número de processos

### Speedup Teórico

```
Speedup = T_serial / T_parallel
        = (2×M×N) / (2×M×N/P + T_comunicação)
```

**Speedup Ideal**: Linear (S = P) quando T_comunicação << T_computação

### Eficiência

```
Eficiência = Speedup / P = T_serial / (P × T_parallel)
```

**Fatores que afetam eficiência:**

1. **Razão computação/comunicação**: Matrizes maiores → melhor eficiência
2. **Latência da rede**: Afeta overhead de inicialização
3. **Largura de banda**: Limita transferência de dados grandes
4. **Balanceamento**: M deve ser divisível por P

## Compilação e Execução

### Linux

```bash
# Compilar
mpicc -o tarefa16 tarefa16.c -lm -Wall -O2

# Executar com diferentes números de processos
mpirun -np 1 ./tarefa16    # Serial
mpirun -np 2 ./tarefa16    # 2 processos
mpirun -np 4 ./tarefa16    # 4 processos
mpirun -np 8 ./tarefa16    # 8 processos
```

### Windows (MS-MPI)

```cmd
REM Compilar (Visual Studio)
cl /I"C:\Program Files (x86)\Microsoft SDKs\MPI\Include" tarefa16.c /link msmpi.lib /out:tarefa16.exe

REM Executar
mpiexec -n 4 tarefa16.exe
```

## Resultados Esperados

### Exemplo de Saída

```
TAREFA 16: PRODUTO MATRIZ-VETOR COM MPI
Implementação: MPI_Scatter + MPI_Bcast + MPI_Gather
Compilação: mpicc -o tarefa16 tarefa16.c -lm
Execução: mpirun -np 4 ./tarefa16

=== PRODUTO MATRIZ-VETOR PARALELO ===
Matriz A: 8x6
Vetor x: 6 elementos
Processos: 4
Linhas por processo: 2

Iniciando cálculo paralelo...
Cálculo paralelo concluído!

Verificação (vs. versão sequencial):
Resultado correto: SIM
Erro máximo: 1.23e-15
Tempo sequencial: 0.000012 s
Speedup: 2.15x

============================================================
BENCHMARK: PRODUTO MATRIZ-VETOR PARALELO
Processos MPI: 4
============================================================

Formato: M x N | Tempo (s) | GFLOPS | Eficiência
--------------------------------------------------
 400 x  400 |   0.0025 |   64.25 |    89.2%
 800 x  800 |   0.0098 |   65.12 |    87.8%
1200 x 1200 |   0.0221 |   65.45 |    86.4%
1600 x 1600 |   0.0395 |   64.89 |    85.1%
2000 x 2000 |   0.0618 |   64.72 |    84.6%
```

### Análise dos Resultados

**GFLOPS (Giga Floating Point Operations per Second):**

- Mede performance computacional bruta
- Valores típicos: 50-100 GFLOPS por core moderno

**Eficiência:**

- > 80%: Excelente paralelização
- 60-80%: Boa paralelização
- < 60%: Overhead significativo

## Otimizações Possíveis

### 1. Blocking e Cache Optimization

```c
// Computação por blocos para melhor uso do cache
#define BLOCK_SIZE 64

for (int ib = 0; ib < rows_per_process; ib += BLOCK_SIZE) {
    for (int jb = 0; jb < N; jb += BLOCK_SIZE) {
        for (int i = ib; i < min(ib + BLOCK_SIZE, rows_per_process); i++) {
            for (int j = jb; j < min(jb + BLOCK_SIZE, N); j++) {
                y_local[i] += A_local[i * N + j] * x[j];
            }
        }
    }
}
```

### 2. Comunicação Assíncrona

```c
// Overlap comunicação com computação
MPI_Request req_bcast, req_scatter;
MPI_Ibcast(x, N, MPI_DOUBLE, 0, MPI_COMM_WORLD, &req_bcast);
MPI_Iscatter(A, rows_per_process * N, MPI_DOUBLE,
             A_local, rows_per_process * N, MPI_DOUBLE,
             0, MPI_COMM_WORLD, &req_scatter);

// Fazer outras tarefas enquanto aguarda...
MPI_Wait(&req_bcast, MPI_STATUS_IGNORE);
MPI_Wait(&req_scatter, MPI_STATUS_IGNORE);
```

### 3. Distribuição 2D

Para matrizes muito grandes, usar decomposição 2D:

- Processos organizados em grade P = Px × Py
- Cada processo possui bloco (M/Px) × (N/Py)
- Reduz comunicação para O(√P) ao invés de O(P)

## Extensões

### 1. Matrizes Esparsas

```c
// Formato CSR (Compressed Sparse Row)
typedef struct {
    double *values;    // Valores não-zero
    int *col_indices; // Índices das colunas
    int *row_ptr;     // Ponteiros para início de cada linha
} sparse_matrix_t;
```

### 2. Precisão Mista

```c
// Usar float para A e double para acumulação
float *A_local_f;
double accumulator;

for (int i = 0; i < rows_per_process; i++) {
    accumulator = 0.0;
    for (int j = 0; j < N; j++) {
        accumulator += (double)A_local_f[i * N + j] * x[j];
    }
    y_local[i] = accumulator;
}
```

### 3. GPU Acceleration

```c
// CUDA + MPI híbrido
#pragma acc parallel loop
for (int i = 0; i < rows_per_process; i++) {
    y_local[i] = 0.0;
    #pragma acc loop reduction(+:y_local[i])
    for (int j = 0; j < N; j++) {
        y_local[i] += A_local[i * N + j] * x[j];
    }
}
```

## Conceitos Importantes

### 1. Coletivas MPI

- **MPI_Bcast**: One-to-all, complexidade O(log P)
- **MPI_Scatter**: One-to-all distribuído, O(log P)
- **MPI_Gather**: All-to-one, O(log P)
- **MPI_Reduce**: All-to-one com operação, O(log P)

### 2. Padrões de Acesso à Memória

- **Row-major**: A[i][j] = A[i*N + j] (C/C++)
- **Column-major**: A[i][j] = A[j*M + i] (Fortran)
- **Cache-friendly**: Acessar dados sequencialmente

### 3. Balanceamento de Carga

- **Estático**: Divisão uniforme (M/P linhas por processo)
- **Dinâmico**: Redistribuição baseada em carga de trabalho
- **Híbrido**: Ajuste automático durante execução

## Conclusões

### Vantagens da Abordagem

1. **Simplicidade**: Uso direto das coletivas MPI
2. **Eficiência**: Comunicação otimizada automaticamente
3. **Escalabilidade**: Funciona bem até centenas de processos
4. **Portabilidade**: Código padrão MPI funciona em qualquer sistema

### Limitações

1. **Divisibilidade**: M deve ser divisível por P
2. **Memória**: Processo 0 precisa armazenar matriz completa
3. **Comunicação**: Gargalo para matrizes pequenas
4. **Topologia**: Não considera hierarquia da rede

### Casos de Uso Ideais

- **Matrizes densas grandes** (M, N > 1000)
- **Razão alta computação/comunicação**
- **Sistemas com boa largura de banda**
- **Aplicações batch** (múltiplos produtos matriz-vetor)

O produto matriz-vetor é uma operação fundamental em álgebra linear computacional e serve como base para algoritmos mais complexos como multiplicação matriz-matriz, sistemas lineares e métodos iterativos.
