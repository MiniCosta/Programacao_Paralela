#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <stdalign.h>
#include <string.h> // Para memset

#define ALIGNMENT 64
#define NUM_TESTS 51  // Número ímpar maior para melhor estatística
#define WARMUP 5      // Execuções de aquecimento
#define CACHE_LINE_SIZE 64 // Tamanho típico de linha de cache

// Estrutura para armazenar resultados estatísticos
typedef struct {
    double times[NUM_TESTS];
    double median;
    double mean;
    double stddev;
    double min;
    double max;
} TimingStats;

// Função para limpar a cache
void clean_cache() {
    const size_t size = 20 * 1024 * 1024; // 20MB (maior que L3)
    volatile char *clean = (volatile char *)malloc(size);
    for (size_t i = 0; i < size; i += CACHE_LINE_SIZE) {
        clean[i] = i;
    }
    free((void*)clean);
}

// Inicializa a matriz e vetores com valores aleatórios
void initialize(double *A, double *x, double *y, int m, int n) {
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            A[i*n + j] = (double)rand() / RAND_MAX;
        }
    }
    
    for (int j = 0; j < n; j++) {
        x[j] = (double)rand() / RAND_MAX;
    }
    
    for (int i = 0; i < m; i++) {
        y[i] = 0.0;
    }
}

// Versão row-major
void matrixVectorRowMajor(double *restrict A, double *restrict x, 
    double *restrict y, int m, int n) {
for (int i = 0; i < m; i++) {
double sum = 0.0;
for (int j = 0; j < n; j++) {
sum += A[i*n + j] * x[j];
}
y[i] = sum;
}
}

// Versão column-major
void matrixVectorColumnMajor(double *restrict A, double *restrict x, 
      double *restrict y, int m, int n) {
for (int i = 0; i < m; i++) {
y[i] = 0.0;
}

for (int j = 0; j < n; j++) {
double xj = x[j];
for (int i = 0; i < m; i++) {
y[i] += A[i*n + j] * xj;
}
}
}

// Funções estatísticas aprimoradas
void calculate_stats(TimingStats *stats) {
    // Calcular média
    double sum = 0.0;
    for (int i = 0; i < NUM_TESTS; i++) {
        sum += stats->times[i];
    }
    stats->mean = sum / NUM_TESTS;

    // Calcular mediana (usando insertion sort para eficiência com pequenos N)
    for (int i = 1; i < NUM_TESTS; i++) {
        double key = stats->times[i];
        int j = i - 1;
        while (j >= 0 && stats->times[j] > key) {
            stats->times[j+1] = stats->times[j];
            j--;
        }
        stats->times[j+1] = key;
    }
    stats->median = stats->times[NUM_TESTS/2];

    // Calcular desvio padrão
    double variance = 0.0;
    for (int i = 0; i < NUM_TESTS; i++) {
        variance += pow(stats->times[i] - stats->mean, 2);
    }
    stats->stddev = sqrt(variance / NUM_TESTS);

    // Min e Max
    stats->min = stats->times[0];
    stats->max = stats->times[NUM_TESTS-1];
}

// Função de medição de tempo aprimorada
TimingStats time_execution(void (*func)(double*, double*, double*, int, int),
                         double *A, double *x, double *y, int m, int n) {
    TimingStats stats;
    struct timeval start, end;

    // Aquecimento mais robusto
    for (int i = 0; i < WARMUP; i++) {
        func(A, x, y, m, n);
        clean_cache();
    }

    // Execuções de medição
    for (int i = 0; i < NUM_TESTS; i++) {
        clean_cache();
        memset(y, 0, m * sizeof(double)); // Reset do vetor de saída

        gettimeofday(&start, NULL);
        func(A, x, y, m, n);
        gettimeofday(&end, NULL);
        
        stats.times[i] = (end.tv_sec - start.tv_sec) * 1e6 + 
                        (end.tv_usec - start.tv_usec);
    }

    calculate_stats(&stats);
    return stats;
}

int main() {
    // Tamanhos mais granulares, especialmente na região crítica
    int sizes[] = {50, 75, 100, 125, 150, 175, 200, 225, 250, 275, 300,
                   350, 400, 450, 500, 550, 600, 700, 800, 900, 1000,
                   1100, 1300, 1500, 1600, 1800, 2000, 2500, 3000};
    int num_sizes = sizeof(sizes)/sizeof(sizes[0]);

    printf("%-6s %-12s %-12s %-8s %-12s %-12s %-8s\n", 
           "N", "Row-M(µs)", "Col-M(µs)", "Ratio", 
           "Row-σ(µs)", "Col-σ(µs)", "σ-Ratio");
    printf("------------------------------------------------------------\n");

    for (int s = 0; s < num_sizes; s++) {
        int m = sizes[s];
        int n = sizes[s];

        // Alocação com verificação de erro
        double *A = (double*)aligned_alloc(ALIGNMENT, m*n*sizeof(double));
        double *x = (double*)aligned_alloc(ALIGNMENT, n*sizeof(double));
        double *y = (double*)aligned_alloc(ALIGNMENT, m*sizeof(double));
        
        if (!A || !x || !y) {
            fprintf(stderr, "Falha na alocação de memória para N=%d\n", m);
            exit(EXIT_FAILURE);
        }

        initialize(A, x, y, m, n);

        // Medição Row-Major
        TimingStats row = time_execution(matrixVectorRowMajor, A, x, y, m, n);
        
        // Reset e medição Column-Major
        memset(y, 0, m * sizeof(double));
        TimingStats col = time_execution(matrixVectorColumnMajor, A, x, y, m, n);

        // Cálculo de razões
        double ratio = col.median / row.median;
        double stddev_ratio = col.stddev / row.stddev;

        printf("%-6d %-12.2f %-12.2f %-8.2f %-12.2f %-12.2f %-8.2f\n", 
               m, row.median, col.median, ratio, 
               row.stddev, col.stddev, stddev_ratio);

        free(A);
        free(x);
        free(y);
    }

    return 0;
}