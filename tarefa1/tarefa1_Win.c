#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

// Macro para indexacao de matriz (mais eficiente)
#define MATRIX_INDEX(matrix, i, j, cols) ((matrix)[(i) * (cols) + (j)])

// Numero de iteracoes para cada teste (primeira descartada)
#define NUM_ITERATIONS 4

// Funcao inline para multiplicacao matriz-vetor com acesso por linhas
static inline void matrix_vector_multiply_rows(double *matrix, double *vector, double *result, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        result[i] = 0.0;
        for (int j = 0; j < cols; j++) {
            result[i] += MATRIX_INDEX(matrix, i, j, cols) * vector[j];
        }
    }
}

// Funcao inline para multiplicacao matriz-vetor com acesso por colunas
static inline void matrix_vector_multiply_cols(double *matrix, double *vector, double *result, int rows, int cols) {
    // Inicializar resultado com zeros
    for (int i = 0; i < rows; i++) {
        result[i] = 0.0;
    }
    
    for (int j = 0; j < cols; j++) {
        for (int i = 0; i < rows; i++) {
            result[i] += MATRIX_INDEX(matrix, i, j, cols) * vector[j];
        }
    }
}

// Funcao para alocar matriz dinamicamente como bloco contiguo
double *allocate_matrix(int rows, int cols) {
    return (double *)malloc(rows * cols * sizeof(double));
}

// Funcao para liberar matriz
void free_matrix(double *matrix) {
    free(matrix);
}

// Funcao para inicializar matriz com valores aleatorios
void initialize_matrix(double *matrix, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            matrix[i * cols + j] = (double)rand() / RAND_MAX;
        }
    }
}

// Funcao para inicializar vetor com valores aleatorios
void initialize_vector(double *vector, int size) {
    for (int i = 0; i < size; i++) {
        vector[i] = (double)rand() / RAND_MAX;
    }
}

// Funcao para medir tempo real de execucao (wall time)
double get_wall_time() {
#ifdef _WIN32
    LARGE_INTEGER time, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&time);
    return (double)time.QuadPart / freq.QuadPart;
#else
    struct timeval time;
    gettimeofday(&time, NULL);
    return time.tv_sec + time.tv_usec * 1e-6;
#endif
}



// Funcao para medir wall time com multiplas iteracoes
double measure_wall_time_multiple(void (*func)(double*, double*, double*, int, int), 
                                 double *matrix, double *vector, double *result, int rows, int cols) {
    double times[NUM_ITERATIONS];
    
    // Executar multiplas iteracoes
    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        double start, end;
        start = get_wall_time();
        func(matrix, vector, result, rows, cols);
        end = get_wall_time();
        times[iter] = end - start;
    }
    
    // Calcular media ignorando a primeira iteracao (aquecimento)
    double sum = 0.0;
    for (int i = 1; i < NUM_ITERATIONS; i++) {
        sum += times[i];
    }
    return sum / (NUM_ITERATIONS - 1);
}

// Funcao para verificar se os resultados sao iguais
int compare_results(double *result1, double *result2, int size) {
    double tolerance = 1e-10;
    for (int i = 0; i < size; i++) {
        if (fabs(result1[i] - result2[i]) > tolerance) {
            return 0; // Resultados diferentes
        }
    }
    return 1; // Resultados iguais
}

// Funcao para executar teste com um tamanho especifico
void run_test(int size) {
    printf("\n");
    printf("=====================================================\n");
    printf("           TESTE COM MATRIZ %dx%d\n", size, size);
    printf("=====================================================\n");
    
    // Alocar memoria
    double *matrix = allocate_matrix(size, size);
    double *vector = (double *)malloc(size * sizeof(double));
    double *result_rows = (double *)malloc(size * sizeof(double));
    double *result_cols = (double *)malloc(size * sizeof(double));
    
    // Inicializar dados
    srand(42); // Seed fixo para resultados reproduziveis
    initialize_matrix(matrix, size, size);
    initialize_vector(vector, size);
    
    // Medir tempo da versao por linhas - Wall Time
    double wall_time_rows = measure_wall_time_multiple(matrix_vector_multiply_rows, matrix, vector, result_rows, size, size);
    
    // Medir tempo da versao por colunas - Wall Time
    double wall_time_cols = measure_wall_time_multiple(matrix_vector_multiply_cols, matrix, vector, result_cols, size, size);
    
    // Verificar se os resultados sao iguais
    if (compare_results(result_rows, result_cols, size)) {
        printf("\n\n");// printf("[OK] Resultados corretos (ambas as versoes produziram o mesmo resultado)\n\n");
    } else {
        printf("[ERRO] Resultados diferentes entre as versoes!\n\n");
    }
    
    // Exibir tempos de forma organizada
    printf("\nTEMPOS DE EXECUCAO:\n");
    printf("-----------------------------------------------------\n");
    printf("                    | Wall Time\n");
    printf("-----------------------------------------------------\n");
    printf("Acesso por linhas   | %.6f s\n", wall_time_rows);
    printf("Acesso por colunas  | %.6f s\n", wall_time_cols);
    printf("-----------------------------------------------------\n");
    
    // Calcular speedups
    printf("\nANALISE DE PERFORMANCE:\n");
    if (wall_time_cols > 0 && wall_time_rows > 0) {
        double wall_speedup = wall_time_cols / wall_time_rows;
        printf("Speedup: %.2fx - ", wall_speedup);
        if (wall_speedup > 1.0) {
            printf("linhas %.2fx mais rapido\n", wall_speedup);
        } else {
            printf("colunas %.2fx mais rapido\n", 1.0/wall_speedup);
        }
        
        // Adicionar análise mais detalhada
        double diff_percent = fabs(wall_speedup - 1.0) * 100.0;
        if (diff_percent > 10.0) {
            printf("Diferenca significativa: %.1f%%\n", diff_percent);
        } else {
            printf("Diferenca pequena: %.1f%%\n", diff_percent);
        }
    }
    
    // Liberar memoria
    free_matrix(matrix);
    free(vector);
    free(result_rows);
    free(result_cols);
}

int main() {
    printf("=== Comparacao de Performance: Multiplicacao Matriz-Vetor ===\n");
    printf("Testando diferentes padroes de acesso a memoria:\n");
    
    
    // Tamanhos de teste - valores ajustados para evitar problemas de memoria
    int sizes[] = {100,200,300,400,500,600,800,1000,1250,1500};
    int num_tests = sizeof(sizes) / sizeof(sizes[0]);
    
    // Executar testes
    for (int i = 0; i < num_tests; i++) {
        run_test(sizes[i]);
    }
    
    return 0;
}
//gcc -O0 -o tarefa1_O0.exe tarefa1.c -lm
// ./tarefa1_O0.exe