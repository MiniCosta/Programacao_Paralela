#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Definições para compatibilidade Windows
#ifdef _WIN32
#include <windows.h>
#include "mpi.h"
#else
#include <mpi.h>
#include <sys/time.h>
#endif

// Configurações da simulação
#define MAX_M 2000      // Máximo número de linhas
#define MAX_N 2000      // Máximo número de colunas
#define NUM_TESTS 5     // Número de testes para média

// Função para obter tempo atual (compatível com Windows)
double get_time() {
#ifdef _WIN32
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
#endif
}

// Função para inicializar matriz com valores aleatórios
void init_matrix(double *A, int M, int N) {
    srand(42); // Seed fixo para reprodutibilidade
    for (int i = 0; i < M * N; i++) {
        A[i] = (double)rand() / RAND_MAX * 10.0 - 5.0; // Valores entre -5 e 5
    }
}

// Função para inicializar vetor com valores aleatórios
void init_vector(double *x, int N) {
    srand(123); // Seed diferente para o vetor
    for (int i = 0; i < N; i++) {
        x[i] = (double)rand() / RAND_MAX * 10.0 - 5.0; // Valores entre -5 e 5
    }
}

// Função para calcular produto matriz-vetor sequencial (para verificação)
void matrix_vector_sequential(double *A, double *x, double *y, int M, int N) {
    for (int i = 0; i < M; i++) {
        y[i] = 0.0;
        for (int j = 0; j < N; j++) {
            y[i] += A[i * N + j] * x[j];
        }
    }
}

// Função principal do produto matriz-vetor paralelo com distribuição por colunas
double matrix_vector_parallel_columns(int rank, int size, int M, int N, int verbose) {
    double *A = NULL;           // Matriz completa (apenas no processo 0)
    double *x = NULL;           // Vetor x completo (apenas no processo 0)
    double *y = NULL;           // Vetor resultado completo (apenas no processo 0)
    double *A_local = NULL;     // Submatriz local (colunas) de cada processo
    double *x_local = NULL;     // Parte local do vetor x
    double *y_local = NULL;     // Contribuição local para o vetor y (tamanho M)
    
    // Verificar se N é divisível pelo número de processos
    if (N % size != 0) {
        if (rank == 0) {
            printf("Erro: N (%d) deve ser divisível pelo número de processos (%d)\n", N, size);
        }
        return -1.0;
    }
    
    int cols_per_process = N / size;  // Colunas por processo
    
    // Alocar memória local
    A_local = (double*)malloc(M * cols_per_process * sizeof(double));
    x_local = (double*)malloc(cols_per_process * sizeof(double));
    y_local = (double*)calloc(M, sizeof(double)); // Inicializar com zeros
    
    // Processo 0: inicializar dados e alocar memória completa
    if (rank == 0) {
        A = (double*)malloc(M * N * sizeof(double));
        x = (double*)malloc(N * sizeof(double));
        y = (double*)malloc(M * sizeof(double));
        
        init_matrix(A, M, N);
        init_vector(x, N);
        
        if (verbose) {
            printf("\n=== PRODUTO MATRIZ-VETOR PARALELO (COLUNAS) ===\n");
            printf("Matriz A: %dx%d\n", M, N);
            printf("Vetor x: %d elementos\n", N);
            printf("Processos: %d\n", size);
            printf("Colunas por processo: %d\n", cols_per_process);
            printf("\nIniciando cálculo paralelo por colunas...\n");
        }
    }
    
    // Criar tipos derivados MPI para distribuição de colunas
    MPI_Datatype column_type, resized_column_type;
    
    // MPI_Type_vector: criar tipo para uma coluna da matriz
    // (count, blocklength, stride, oldtype, newtype)
    MPI_Type_vector(M, 1, N, MPI_DOUBLE, &column_type);
    
    // MPI_Type_create_resized: redimensionar para permitir scatter correto
    // O extent deve ser o tamanho de uma coluna (sizeof(double))
    MPI_Type_create_resized(column_type, 0, sizeof(double), &resized_column_type);
    
    // Fazer commit dos tipos
    MPI_Type_commit(&resized_column_type);
    
    // Sincronizar todos os processos antes de começar a medição
    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = get_time();
    
    // 1. SCATTER: Distribuir colunas da matriz A entre os processos
    MPI_Scatter(A, cols_per_process, resized_column_type,
                A_local, M * cols_per_process, MPI_DOUBLE,
                0, MPI_COMM_WORLD);
    
    // 2. SCATTER: Distribuir segmentos correspondentes de x
    MPI_Scatter(x, cols_per_process, MPI_DOUBLE,
                x_local, cols_per_process, MPI_DOUBLE,
                0, MPI_COMM_WORLD);
    
    // 3. COMPUTAÇÃO LOCAL: Cada processo calcula sua contribuição parcial para y
    // Usando as colunas locais da matriz
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < cols_per_process; j++) {
            y_local[i] += A_local[i * cols_per_process + j] * x_local[j];
        }
    }
    
    // 4. REDUCE: Somar as contribuições parciais usando MPI_Reduce
    MPI_Reduce(y_local, y, M, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    
    // Sincronizar todos os processos após a computação
    MPI_Barrier(MPI_COMM_WORLD);
    double end_time = get_time();
    
    double elapsed_time = end_time - start_time;
    
    // Limpeza dos tipos MPI
    MPI_Type_free(&resized_column_type);
    MPI_Type_free(&column_type);
    
    // Processo 0: verificação e limpeza
    if (rank == 0) {
        if (verbose) {
            printf("Cálculo paralelo por colunas concluído!\n");
            
            // Verificação com versão sequencial (apenas para matrizes pequenas)
            if (M <= 500 && N <= 500) {
                double *y_seq = (double*)malloc(M * sizeof(double));
                double start_seq = get_time();
                matrix_vector_sequential(A, x, y_seq, M, N);
                double end_seq = get_time();
                
                // Verificar se os resultados são iguais
                int correct = 1;
                double max_error = 0.0;
                for (int i = 0; i < M; i++) {
                    double error = fabs(y[i] - y_seq[i]);
                    if (error > max_error) max_error = error;
                    if (error > 1e-10) correct = 0;
                }
                
                printf("\nVerificação (vs. versão sequencial):\n");
                printf("Resultado correto: %s\n", correct ? "SIM" : "NÃO");
                printf("Erro máximo: %.2e\n", max_error);
                printf("Tempo sequencial: %.6f s\n", end_seq - start_seq);
                printf("Speedup: %.2fx\n", (end_seq - start_seq) / elapsed_time);
                
                free(y_seq);
            }
        }
        
        free(A);
        free(x);
        free(y);
    }
    
    // Limpeza da memória local
    free(A_local);
    free(x_local);
    free(y_local);
    
    return elapsed_time;
}

// Função para executar benchmark com diferentes tamanhos
void run_benchmark(int rank, int size) {
    if (rank == 0) {
        printf("\n=====================================\n");
        printf("BENCHMARK: PRODUTO MATRIZ-VETOR (COLUNAS)\n");
        printf("Processos MPI: %d\n", size);
        printf("=====================================\n");
    }
    
    // Diferentes tamanhos para teste
    int test_sizes[][2] = {
        {400, 400},     // Pequeno
        {800, 800},     // Médio
        {1200, 1200},   // Grande
        {1600, 1600},   // Muito grande
        {2000, 2000}    // Máximo
    };
    int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    if (rank == 0) {
        printf("\nFormato: M x N | Tempo (s) | GFLOPS | Eficiência\n");
        printf("--------------------------------------------------\n");
    }
    
    for (int test = 0; test < num_sizes; test++) {
        int M = test_sizes[test][0];
        int N = test_sizes[test][1];
        
        // Verificar se N é divisível pelo número de processos
        if (N % size != 0) {
            if (rank == 0) {
                printf("%4d x %4d | SKIP (N não divisível por %d)\n", M, N, size);
            }
            continue;
        }
        
        // Executar múltiplos testes para obter média
        double total_time = 0.0;
        int valid_tests = 0;
        
        for (int run = 0; run < NUM_TESTS; run++) {
            double time = matrix_vector_parallel_columns(rank, size, M, N, 0);
            if (time > 0) {
                total_time += time;
                valid_tests++;
            }
        }
        
        if (valid_tests > 0 && rank == 0) {
            double avg_time = total_time / valid_tests;
            
            // Calcular GFLOPS: 2*M*N operações (mult + add por elemento)
            double gflops = (2.0 * M * N) / (avg_time * 1e9);
            
            // Eficiência aproximada (considerando speedup linear ideal)
            double efficiency = 100.0; // Para 1 processo, eficiência = 100%
            if (size > 1) {
                // Estimar eficiência baseada na razão computação/comunicação
                double comm_overhead = (double)(M + N) / (M * N); // Aproximação
                efficiency = 100.0 * (1.0 - comm_overhead * size);
                if (efficiency > 100.0) efficiency = 100.0;
                if (efficiency < 0.0) efficiency = 0.0;
            }
            
            printf("%4d x %4d | %8.4f | %6.2f | %7.1f%%\n", 
                   M, N, avg_time, gflops, efficiency);
            fflush(stdout);
        }
        
        MPI_Barrier(MPI_COMM_WORLD);
    }
}

// Função para demonstração detalhada
void run_detailed_demo(int rank, int size) {
    if (rank == 0) {
        printf("\n=====================================\n");
        printf("DEMONSTRAÇÃO DETALHADA\n");
        printf("=====================================\n");
    }
    
    // Teste com matriz pequena para mostrar detalhes
    int M = 8, N = 6;
    
    // Ajustar tamanho se necessário para ser divisível
    while (N % size != 0) N++;
    
    if (rank == 0) {
        printf("Executando demonstração com matriz %dx%d...\n", M, N);
    }
    
    double time = matrix_vector_parallel_columns(rank, size, M, N, 1);
    
    if (rank == 0 && time > 0) {
        printf("\nTempo total (paralelo): %.6f segundos\n", time);
        double gflops = (2.0 * M * N) / (time * 1e9);
        printf("Performance: %.2f GFLOPS\n", gflops);
    }
}

int main(int argc, char *argv[]) {
    int rank, size;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (rank == 0) {
        printf("TAREFA 17: PRODUTO MATRIZ-VETOR COM MPI (DISTRIBUIÇÃO POR COLUNAS)\n");
        printf("Implementação: MPI_Type_vector + MPI_Type_create_resized + MPI_Scatter + MPI_Reduce\n");
        printf("Compilação: mpicc -o tarefa17 tarefa17.c -lm\n");
        printf("Execução: mpirun -np %d ./tarefa17\n", size);
    }
    
    // Executar demonstração detalhada
    run_detailed_demo(rank, size);
    
    // Executar benchmark de performance
    run_benchmark(rank, size);
    
    if (rank == 0) {
        printf("\n=====================================\n");
        printf("ANÁLISE COMPARATIVA: LINHAS vs COLUNAS\n");
        printf("=====================================\n");
        
        printf("\n*** DISTRIBUIÇÃO POR LINHAS (Tarefa 16) ***\n");
        printf("• Comunicação:\n");
        printf("  - MPI_Bcast: Distribui vetor x completo (N elementos)\n");
        printf("  - MPI_Scatter: Distribui linhas da matriz (M*N/P elementos)\n");
        printf("  - MPI_Gather: Coleta resultados parciais (M/P elementos)\n");
        printf("  - Total comunicado: N + M*N/P + M/P elementos\n");
        
        printf("\n• Padrão de acesso à memória:\n");
        printf("  - Acesso sequencial às linhas da matriz (bom para cache)\n");
        printf("  - Cada processo acessa x[0..N-1] sequencialmente\n");
        printf("  - Localidade espacial boa na matriz A\n");
        
        printf("\n• Computação:\n");
        printf("  - Cada processo calcula M/P elementos do resultado\n");
        printf("  - Carga de trabalho: (M/P) * N operações por processo\n");
        
        printf("\n*** DISTRIBUIÇÃO POR COLUNAS (Tarefa 17) ***\n");
        printf("• Comunicação:\n");
        printf("  - MPI_Scatter (colunas): Distribui colunas da matriz (M*N/P elementos)\n");
        printf("  - MPI_Scatter (x): Distribui segmentos de x (N/P elementos)\n");
        printf("  - MPI_Reduce: Soma contribuições parciais (M elementos)\n");
        printf("  - Total comunicado: M*N/P + N/P + M elementos\n");
        
        printf("\n• Padrão de acesso à memória:\n");
        printf("  - Acesso com stride N na matriz original (pior para cache)\n");
        printf("  - Após scatter, acesso sequencial às colunas locais\n");
        printf("  - Localidade espacial menor durante a distribuição\n");
        
        printf("\n• Computação:\n");
        printf("  - Cada processo calcula contribuição para todos M elementos\n");
        printf("  - Carga de trabalho: M * (N/P) operações por processo\n");
        
        printf("\n*** COMPARAÇÃO DE DESEMPENHO ***\n");
        printf("• Volume de comunicação:\n");
        printf("  - Linhas: N + M*N/P + M/P ≈ N + M*N/P (dominante)\n");
        printf("  - Colunas: M*N/P + N/P + M ≈ M*N/P (similar)\n");
        printf("  - Ambos têm O(M*N/P) de dados transferidos\n");
        
        printf("\n• Eficiência de cache:\n");
        printf("  - Linhas: Melhor (acesso sequencial)\n");
        printf("  - Colunas: Pior durante scatter, melhor após reorganização\n");
        
        printf("\n• Sincronização:\n");
        printf("  - Linhas: MPI_Gather (simples coleta)\n");
        printf("  - Colunas: MPI_Reduce (operação de redução)\n");
        
        printf("\n• Escalabilidade:\n");
        printf("  - Linhas: Limitada por M (precisa M ≥ P)\n");
        printf("  - Colunas: Limitada por N (precisa N ≥ P)\n");
        
        printf("\n*** QUANDO USAR CADA ABORDAGEM ***\n");
        printf("• Distribuição por linhas:\n");
        printf("  - Matrizes com M >> N (muitas linhas, poucas colunas)\n");
        printf("  - Quando cache hit rate é crítico\n");
        printf("  - Aplicações que reutilizam o vetor x\n");
        
        printf("\n• Distribuição por colunas:\n");
        printf("  - Matrizes com N >> M (poucas linhas, muitas colunas)\n");
        printf("  - Quando se deseja paralelizar operações de redução\n");
        printf("  - Aplicações que fazem múltiplos produtos com diferentes x\n");
        
        printf("\n*** TIPOS DERIVADOS MPI ***\n");
        printf("• MPI_Type_vector: Define padrão de acesso não contíguo\n");
        printf("  - count=%d (elementos por coluna)\n", MAX_M);
        printf("  - blocklength=1 (um elemento por bloco)\n");
        printf("  - stride=N (distância entre elementos)\n");
        
        printf("\n• MPI_Type_create_resized: Ajusta extent do tipo\n");
        printf("  - Permite scatter correto de múltiplas colunas\n");
        printf("  - Extent=sizeof(double) para colunas adjacentes\n");
        
        printf("=====================================\n");
    }
    
    MPI_Finalize();
    return 0;
}
