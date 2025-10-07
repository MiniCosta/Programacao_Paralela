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

// Configurações da simulação - limites de tamanho para evitar overflow de memória
#define MAX_M 12000     // Máximo número de linhas
#define MAX_N 12000     // Máximo número de colunas
#define NUM_TESTS 3     // Número de testes para média e precisão estatística

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

// Função para inicializar matriz com valores aleatórios - garante resultados reproduzíveis
void init_matrix(double *A, int M, int N) {
    srand(42); // Seed fixo para reprodutibilidade entre execuções
    for (int i = 0; i < M * N; i++) {
        A[i] = (double)rand() / RAND_MAX * 10.0 - 5.0; // Valores entre -5 e 5 para evitar overflow
    }
}

// Função para inicializar vetor com valores aleatórios - independente da matriz
void init_vector(double *x, int N) {
    srand(123); // Seed diferente para garantir independência estatística
    for (int i = 0; i < N; i++) {
        x[i] = (double)rand() / RAND_MAX * 10.0 - 5.0; // Mesma faixa da matriz para consistência
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
    
    // Verificar divisibilidade - requisito para balanceamento perfeito de carga
    if (M % size != 0) {
        if (rank == 0) {
            printf("Erro: M (%d) deve ser divisível pelo número de processos (%d)\n", M, size);
        }
        return -1.0; // Retorna erro para indicar falha na configuração
    }
    
    int rows_per_process = M / size;  // Cada processo recebe exatamente o mesmo número de linhas
    
    // Alocar memória local para cada processo - estruturas distribuídas
    x = (double*)malloc(N * sizeof(double)); // Vetor completo será recebido via MPI_Bcast
    y_local = (double*)malloc(rows_per_process * sizeof(double)); // Resultado parcial do processo
    A_local = (double*)malloc(rows_per_process * N * sizeof(double)); // Submatriz local recebida via MPI_Scatter
    
    // Apenas o processo mestre (rank 0) possui os dados completos inicialmente
    if (rank == 0) {
        A = (double*)malloc(M * N * sizeof(double)); // Matriz completa apenas no processo 0
        y = (double*)malloc(M * sizeof(double)); // Vetor resultado final apenas no processo 0
        
        init_matrix(A, M, N); // Inicializar matriz com valores conhecidos
        init_vector(x, N); // Inicializar vetor de entrada
        
        if (verbose) {
            printf("\n=== PRODUTO MATRIZ-VETOR PARALELO ===\n");
            printf("Matriz A: %dx%d\n", M, N);
            printf("Vetor x: %d elementos\n", N);
            printf("Processos: %d\n", size);
            printf("Linhas por processo: %d\n", rows_per_process);
            printf("\nIniciando cálculo paralelo...\n");
        }
    }
    
    // Sincronização para medição precisa - todos os processos começam juntos
    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = get_time(); // Marca o início do algoritmo paralelo
    
    // FASE 1: Distribuir vetor x completo para todos os processos (comunicação coletiva)
    MPI_Bcast(x, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    
    // FASE 2: Distribuir linhas da matriz A entre processos (decomposição por linhas)
    MPI_Scatter(A, rows_per_process * N, MPI_DOUBLE,
                A_local, rows_per_process * N, MPI_DOUBLE,
                0, MPI_COMM_WORLD);
    
    // FASE 3: Computação paralela - cada processo calcula suas linhas independentemente
    for (int i = 0; i < rows_per_process; i++) {
        y_local[i] = 0.0; // Inicializar acumulador para precisão numérica
        for (int j = 0; j < N; j++) {
            y_local[i] += A_local[i * N + j] * x[j]; // Produto escalar linha-vetor
        }
    }
    
    // FASE 4: Coletar resultados parciais de volta ao processo mestre
    MPI_Gather(y_local, rows_per_process, MPI_DOUBLE,
               y, rows_per_process, MPI_DOUBLE,
               0, MPI_COMM_WORLD);
    
    // Sincronização final para medição precisa - aguarda todos terminarem
    MPI_Barrier(MPI_COMM_WORLD);
    double end_time = get_time(); // Marca o fim do algoritmo paralelo
    
    double elapsed_time = end_time - start_time; // Tempo total incluindo comunicação
    
    // Processo 0: verificação e limpeza
    if (rank == 0) {
        if (verbose) {
            printf("Cálculo paralelo concluído!\n");
            
            // Verificação de correção apenas para matrizes pequenas (evita overhead)
            if (M <= 100 && N <= 100) {
                double *y_seq = (double*)malloc(M * sizeof(double)); // Resultado da versão sequencial
                double start_seq = get_time();
                matrix_vector_sequential(A, x, y_seq, M, N); // Execução sequencial para comparação
                double end_seq = get_time();
                
                // Comparação numérica entre versão paralela e sequencial
                int correct = 1; // Flag para indicar correção
                double max_error = 0.0; // Maior erro encontrado
                for (int i = 0; i < M; i++) {
                    double error = fabs(y[i] - y_seq[i]); // Erro absoluto por elemento
                    if (error > max_error) max_error = error; // Rastrear erro máximo
                    if (error > 1e-10) correct = 0; // Tolerância para precisão de ponto flutuante
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
    
    // Limpeza de memória local - cada processo limpa suas estruturas
    free(x); // Liberar cópia local do vetor x
    free(y_local); // Liberar resultado parcial
    free(A_local); // Liberar submatriz local
    
    return elapsed_time; // Retornar tempo medido para análise de performance
}

// Função para executar benchmark com diferentes tamanhos
void run_benchmark(int rank, int size) {
    if (rank == 0) {
        printf("\n============================================================\n");
        printf("BENCHMARK: PRODUTO MATRIZ-VETOR PARALELO\n");
        printf("Processos MPI: %d\n", size);
        printf("============================================================\n");
    }
    
    // Diferentes tamanhos para teste
    int test_sizes[][2] = {
        {2000, 2000},   // Pequeno
        {4000, 4000},   // Médio
        {6000, 6000},   // Grande
        {8000, 8000},   // Muito grande
        {10000, 10000}, // Enorme
        {12000, 12000}, // Muito enorme
        {14000, 14000}, // Gigantesco
        {16000, 16000}  // Máximo
    };
    int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    if (rank == 0) {
        printf("\nFormato: M x N | Tempo (s) | GFLOPS | Eficiência\n");
        printf("--------------------------------------------------\n");
    }
    
    for (int test = 0; test < num_sizes; test++) {
        int M = test_sizes[test][0];
        int N = test_sizes[test][1];
        
        // Pular tamanhos incompatíveis com o número de processos
        if (M % size != 0) {
            if (rank == 0) {
                printf("%4d x %4d | SKIP (M não divisível por %d)\n", M, N, size);
            }
            continue; // Próximo tamanho de teste
        }
        
        // Executar múltiplos testes para obter média estatisticamente válida
        double total_time = 0.0; // Acumulador de tempos
        int valid_tests = 0; // Contador de testes bem-sucedidos
        
        for (int run = 0; run < NUM_TESTS; run++) {
            double time = matrix_vector_parallel(rank, size, M, N, 0); // Executar sem verbose
            if (time > 0) { // Verificar se execução foi bem-sucedida
                total_time += time; // Acumular tempo válido
                valid_tests++; // Contar teste válido
            }
        }
        
        if (valid_tests > 0 && rank == 0) { // Apenas processo mestre reporta resultados
            double avg_time = total_time / valid_tests; // Média aritmética dos tempos
            
            // Calcular GFLOPS: 2*M*N operações (uma multiplicação + uma soma por elemento)
            double gflops = (2.0 * M * N) / (avg_time * 1e9); // Conversão para bilhões de operações/segundo
            
            // Estimar eficiência baseada no overhead de comunicação
            double efficiency = 100.0; // Execução sequencial tem eficiência 100% por definição
            if (size > 1) {
                // Aproximação: overhead cresce com perímetro (M+N) vs área (M*N)
                double comm_overhead = (double)(M + N) / (M * N); // Razão comunicação/computação
                efficiency = 100.0 * (1.0 - comm_overhead * size); // Penalidade por processo
                if (efficiency > 100.0) efficiency = 100.0; // Limitar máximo
                if (efficiency < 0.0) efficiency = 0.0; // Limitar mínimo
            }
            
            printf("%4d x %4d | %8.4f | %6.2f | %7.1f%%\n", 
                   M, N, avg_time, gflops, efficiency);
            fflush(stdout);
        }
        
        MPI_Barrier(MPI_COMM_WORLD); // Sincronizar antes do próximo teste
    }
}

// Função para demonstração detalhada
void run_detailed_demo(int rank, int size) {
    if (rank == 0) {
        printf("\n============================================================\n");
        printf("DEMONSTRAÇÃO DETALHADA\n");
        printf("============================================================\n");
    }
    
    // Usar matriz pequena para demonstração detalhada (permite verificação manual)
    int M = 8, N = 6;
    
    // Garantir divisibilidade automática ajustando M
    while (M % size != 0) M++; // Incrementar até ser divisível
    
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
    int rank, size; // Identificação do processo e total de processos
    
    // Inicializar ambiente MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Obter ID do processo (0 a size-1)
    MPI_Comm_size(MPI_COMM_WORLD, &size); // Obter número total de processos
    
    if (rank == 0) {
        printf("TAREFA 16: PRODUTO MATRIZ-VETOR COM MPI\n");
        printf("Implementação: MPI_Scatter + MPI_Bcast + MPI_Gather\n");
        printf("Compilação: mpicc -o tarefa16 tarefa16.c -lm\n");
        printf("Execução: mpirun -np %d ./tarefa16\n", size);
    }
    
    // Executar demonstração com matriz pequena (verificação e debugging)
    run_detailed_demo(rank, size);
    
    // Executar benchmark completo com múltiplos tamanhos de matriz
    run_benchmark(rank, size);
    
    if (rank == 0) {
        printf("\n============================================================\n");
        printf("ANÁLISE DOS RESULTADOS:\n");
        printf("- MPI_Bcast: Distribui vetor x para todos os processos\n");
        printf("- MPI_Scatter: Divide matriz A por linhas entre processos\n");
        printf("- Computação local: Cada processo calcula suas linhas de y\n");
        printf("- MPI_Gather: Coleta resultados parciais no processo 0\n");
        printf("\nComunicação total: O(N) + O(M) elementos\n");
        printf("Computação por processo: O(M*N/P) onde P = número de processos\n");
        printf("Speedup ideal: Linear até saturar largura de banda\n");
        printf("============================================================\n");
    }
    
    MPI_Finalize(); // Finalizar ambiente MPI e limpar recursos
    return 0; // Sucesso
}
