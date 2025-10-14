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
    for (int i = 0; i < rows; i++) { // Percorre linhas da matriz (row-major order)
        result[i] = 0.0; // Inicializa elemento do resultado
        for (int j = 0; j < cols; j++) { // Percorre colunas da linha atual
            result[i] += MATRIX_INDEX(matrix, i, j, cols) * vector[j]; // Acumula produto escalar
        }
    }
}

// Funcao inline para multiplicacao matriz-vetor com acesso por colunas
static inline void matrix_vector_multiply_cols(double *matrix, double *vector, double *result, int rows, int cols) {
    // Inicializar resultado com zeros
    for (int i = 0; i < rows; i++) { // Zera vetor resultado
        result[i] = 0.0;
    }
    
    for (int j = 0; j < cols; j++) { // Percorre colunas primeiro (column-major access)
        for (int i = 0; i < rows; i++) { // Percorre linhas para cada coluna (saltos de memória)
            result[i] += MATRIX_INDEX(matrix, i, j, cols) * vector[j]; // Acesso com stride = cols
        }
    }
}

// Funcao para alocar matriz dinamicamente como bloco contiguo
double *allocate_matrix(int rows, int cols) {
    return (double *)malloc(rows * cols * sizeof(double)); // Aloca memória contígua para a matriz
}

// Funcao para liberar matriz
void free_matrix(double *matrix) {
    free(matrix); // Libera memória alocada dinamicamente
}

// Funcao para inicializar matriz com valores aleatorios
void initialize_matrix(double *matrix, int rows, int cols) {
    for (int i = 0; i < rows; i++) { // Percorre linhas
        for (int j = 0; j < cols; j++) { // Percorre colunas
            matrix[i * cols + j] = (double)rand() / RAND_MAX; // Preenche com valores aleatórios [0,1]
        }
    }
}

// Funcao para inicializar vetor com valores aleatorios
void initialize_vector(double *vector, int size) {
    for (int i = 0; i < size; i++) { // Percorre elementos do vetor
        vector[i] = (double)rand() / RAND_MAX; // Preenche com valores aleatórios [0,1]
    }
}

// Funcao para medir tempo real de execucao (wall time)
double get_wall_time() {
#ifdef _WIN32
    LARGE_INTEGER time, freq; // Estruturas para high-resolution timer no Windows
    QueryPerformanceFrequency(&freq); // Obtém frequência do contador
    QueryPerformanceCounter(&time); // Obtém valor atual do contador
    return (double)time.QuadPart / freq.QuadPart; // Retorna tempo em segundos
#else
    struct timeval time; // Estrutura para tempo no Unix/Linux
    gettimeofday(&time, NULL); // Obtém tempo atual com precisão de microssegundos
    return time.tv_sec + time.tv_usec * 1e-6; // Converte para segundos
#endif
}



// Funcao para medir wall time com multiplas iteracoes
double measure_wall_time_multiple(void (*func)(double*, double*, double*, int, int), 
                                 double *matrix, double *vector, double *result, int rows, int cols) {
    double times[NUM_ITERATIONS]; // Array para armazenar tempos de cada iteração
    
    // Executar multiplas iteracoes
    for (int iter = 0; iter < NUM_ITERATIONS; iter++) { // Loop de iterações para obter média
        double start, end; // Variáveis para tempo inicial e final
        start = get_wall_time(); // Marca tempo inicial
        func(matrix, vector, result, rows, cols); // Executa função a ser medida
        end = get_wall_time(); // Marca tempo final
        times[iter] = end - start; // Calcula tempo decorrido
    }
    
    // Calcular media ignorando a primeira iteracao (aquecimento)
    double sum = 0.0; // Soma dos tempos (exceto primeira iteração)
    for (int i = 1; i < NUM_ITERATIONS; i++) { // Ignora primeira iteração (warm-up)
        sum += times[i]; // Acumula tempos
    }
    return sum / (NUM_ITERATIONS - 1); // Retorna média dos tempos válidos
}

// Funcao para verificar se os resultados sao iguais
int compare_results(double *result1, double *result2, int size) {
    double tolerance = 1e-10; // Tolerância para comparação de ponto flutuante
    for (int i = 0; i < size; i++) { // Compara elemento por elemento
        if (fabs(result1[i] - result2[i]) > tolerance) { // Verifica diferença absoluta
            return 0; // Resultados diferentes
        }
    }
    return 1; // Resultados iguais dentro da tolerância
}

// Funcao para executar teste com um tamanho especifico
void run_test(int size) {
    printf("\n"); // Linha em branco para separação visual
    printf("=====================================================\n");
    printf("           TESTE COM MATRIZ %dx%d\n", size, size);
    printf("=====================================================");
    
    // Alocar memoria
    double *matrix = allocate_matrix(size, size); // Matriz quadrada NxN
    double *vector = (double *)malloc(size * sizeof(double)); // Vetor de entrada
    double *result_rows = (double *)malloc(size * sizeof(double)); // Resultado row-major
    double *result_cols = (double *)malloc(size * sizeof(double)); // Resultado column-major
    
    // Inicializar dados
    srand(42); // Seed fixo para resultados reproduziveis entre execuções
    initialize_matrix(matrix, size, size); // Preenche matriz com valores aleatórios
    initialize_vector(vector, size); // Preenche vetor com valores aleatórios
    
    // Medir tempo da versao por linhas - Wall Time
    double wall_time_rows = measure_wall_time_multiple(matrix_vector_multiply_rows, matrix, vector, result_rows, size, size); // Acesso row-major
    
    // Medir tempo da versao por colunas - Wall Time
    double wall_time_cols = measure_wall_time_multiple(matrix_vector_multiply_cols, matrix, vector, result_cols, size, size); // Acesso column-major
    
    // Verificar se os resultados sao iguais
    if (compare_results(result_rows, result_cols, size)) { // Validação da correção dos algoritmos
        printf("\n\n");// printf("[OK] Resultados corretos (ambas as versoes produziram o mesmo resultado)\n\n");
    } else {
        printf("[ERRO] Resultados diferentes entre as versoes!\n\n"); // Indica erro na implementação
    }
    
    // Exibir tempos de forma organizada
    printf("TEMPOS DE EXECUCAO:\n");
    printf("-----------------------------------------------------\n");
    printf("                    | Wall Time\n");
    printf("-----------------------------------------------------\n");
    printf("Acesso por linhas   | %.6f s\n", wall_time_rows);
    printf("Acesso por colunas  | %.6f s\n", wall_time_cols);
    printf("-----------------------------------------------------\n");
    
    // Calcular speedups
    printf("\nANALISE DE PERFORMANCE:\n");
    if (wall_time_cols > 0 && wall_time_rows > 0) { // Evita divisão por zero
        double wall_speedup = wall_time_cols / wall_time_rows; // Calcula fator de aceleração
        // printf("Speedup: %.2fx - ", wall_speedup);
        if (wall_speedup > 1.0) { // Row-major mais rápido (caso esperado)
            printf("linhas %.2fx mais rapido\n", wall_speedup);
        } else { // Column-major mais rápido (caso inesperado)
            printf("colunas %.2fx mais rapido\n", 1.0/wall_speedup);
        }
        
    }
    
    // Liberar memoria
    free_matrix(matrix); // Libera matriz
    free(vector); // Libera vetor de entrada
    free(result_rows); // Libera resultado row-major
    free(result_cols); // Libera resultado column-major
}

int main() {
    printf("=== Comparacao de Performance: Multiplicacao Matriz-Vetor ===\n");
    printf("Testando diferentes padroes de acesso a memoria:\n");
    
    
    // Tamanhos de teste - valores expandidos para análise mais ampla de performance
    int sizes[] = {200, 400, 600, 800, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 5000};
    int num_tests = sizeof(sizes) / sizeof(sizes[0]); // Calcula número de testes automaticamente
    
    // Executar testes
    for (int i = 0; i < num_tests; i++) { // Loop através de todos os tamanhos de teste
        run_test(sizes[i]); // Executa teste para tamanho específico
    }
    
    return 0; // Indica execução bem-sucedida
}
//gcc -O0 -o tarefa1_O0.exe tarefa1.c -lm
// ./tarefa1_O0.exe