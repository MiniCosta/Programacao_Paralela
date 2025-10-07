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

// Função principal do produto matriz-vetor paralelo
double matrix_vector_parallel(int rank, int size, int M, int N, int verbose) {
    double *A = NULL;           // Matriz completa (apenas no processo 0)
    double *x = NULL;           // Vetor x completo (todos os processos)
    double *y = NULL;           // Vetor resultado completo (apenas no processo 0)
    double *A_local = NULL;     // Submatriz local de cada processo
    double *y_local = NULL;     // Parte local do vetor resultado
    
    // Verificar se M é divisível pelo número de processos
    if (M % size != 0) {
        if (rank == 0) {
            printf("Erro: M (%d) deve ser divisível pelo número de processos (%d)\n", M, size);
        }
        return -1.0;
    }
    
    int rows_per_process = M / size;  // Linhas por processo
    
    // Alocar memória para vetores (todos os processos precisam de x)
    x = (double*)malloc(N * sizeof(double));
    y_local = (double*)malloc(rows_per_process * sizeof(double));
    A_local = (double*)malloc(rows_per_process * N * sizeof(double));
    
    // Processo 0: inicializar dados e alocar memória completa
    if (rank == 0) {
        A = (double*)malloc(M * N * sizeof(double));
        y = (double*)malloc(M * sizeof(double));
        
        init_matrix(A, M, N);
        init_vector(x, N);
        
        if (verbose) {
            printf("\n=== PRODUTO MATRIZ-VETOR PARALELO ===\n");
            printf("Matriz A: %dx%d\n", M, N);
            printf("Vetor x: %d elementos\n", N);
            printf("Processos: %d\n", size);
            printf("Linhas por processo: %d\n", rows_per_process);
            printf("\nIniciando cálculo paralelo...\n");
        }
    }
    
    // Sincronizar todos os processos antes de começar a medição
    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = get_time();
    
    // 1. BROADCAST: Distribuir vetor x para todos os processos
    MPI_Bcast(x, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    
    // 2. SCATTER: Distribuir linhas da matriz A entre os processos
    MPI_Scatter(A, rows_per_process * N, MPI_DOUBLE,
                A_local, rows_per_process * N, MPI_DOUBLE,
                0, MPI_COMM_WORLD);
    
    // 3. COMPUTAÇÃO LOCAL: Cada processo calcula sua parte de y
    for (int i = 0; i < rows_per_process; i++) {
        y_local[i] = 0.0;
        for (int j = 0; j < N; j++) {
            y_local[i] += A_local[i * N + j] * x[j];
        }
    }
    
    // 4. GATHER: Coletar resultados parciais no processo 0
    MPI_Gather(y_local, rows_per_process, MPI_DOUBLE,
               y, rows_per_process, MPI_DOUBLE,
               0, MPI_COMM_WORLD);
    
    // Sincronizar todos os processos após a computação
    MPI_Barrier(MPI_COMM_WORLD);
    double end_time = get_time();
    
    double elapsed_time = end_time - start_time;
    
    // Processo 0: verificação e limpeza
    if (rank == 0) {
        if (verbose) {
            printf("Cálculo paralelo concluído!\n");
            
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
        free(y);
    }
    
    // Limpeza da memória local
    free(x);
    free(y_local);
    free(A_local);
    
    return elapsed_time;
}

// Função para executar benchmark com diferentes tamanhos
void run_benchmark(int rank, int size) {
    if (rank == 0) {
        printf("\n" "="*60 "\n");
        printf("BENCHMARK: PRODUTO MATRIZ-VETOR PARALELO\n");
        printf("Processos MPI: %d\n", size);
        printf("=" "*60" "\n");
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
        printf("-" "*50" "\n");
    }
    
    for (int test = 0; test < num_sizes; test++) {
        int M = test_sizes[test][0];
        int N = test_sizes[test][1];
        
        // Verificar se M é divisível pelo número de processos
        if (M % size != 0) {
            if (rank == 0) {
                printf("%4d x %4d | SKIP (M não divisível por %d)\n", M, N, size);
            }
            continue;
        }
        
        // Executar múltiplos testes para obter média
        double total_time = 0.0;
        int valid_tests = 0;
        
        for (int run = 0; run < NUM_TESTS; run++) {
            double time = matrix_vector_parallel(rank, size, M, N, 0);
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
        printf("\n" "="*60 "\n");
        printf("DEMONSTRAÇÃO DETALHADA\n");
        printf("=" "*60" "\n");
    }
    
    // Teste com matriz pequena para mostrar detalhes
    int M = 8, N = 6;
    
    // Ajustar tamanho se necessário para ser divisível
    while (M % size != 0) M++;
    
    if (rank == 0) {
        printf("Executando demonstração com matriz %dx%d...\n", M, N);
    }
    
    double time = matrix_vector_parallel(rank, size, M, N, 1);
    
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
        printf("TAREFA 16: PRODUTO MATRIZ-VETOR COM MPI\n");
        printf("Implementação: MPI_Scatter + MPI_Bcast + MPI_Gather\n");
        printf("Compilação: mpicc -o tarefa16 tarefa16.c -lm\n");
        printf("Execução: mpirun -np %d ./tarefa16\n", size);
    }
    
    // Executar demonstração detalhada
    run_detailed_demo(rank, size);
    
    // Executar benchmark de performance
    run_benchmark(rank, size);
    
    if (rank == 0) {
        printf("\n" "="*60 "\n");
        printf("ANÁLISE DOS RESULTADOS:\n");
        printf("- MPI_Bcast: Distribui vetor x para todos os processos\n");
        printf("- MPI_Scatter: Divide matriz A por linhas entre processos\n");
        printf("- Computação local: Cada processo calcula suas linhas de y\n");
        printf("- MPI_Gather: Coleta resultados parciais no processo 0\n");
        printf("\nComunicação total: O(N) + O(M) elementos\n");
        printf("Computação por processo: O(M*N/P) onde P = número de processos\n");
        printf("Speedup ideal: Linear até saturar largura de banda\n");
        printf("=" "*60" "\n");
    }
    
    MPI_Finalize();
    return 0;
}
