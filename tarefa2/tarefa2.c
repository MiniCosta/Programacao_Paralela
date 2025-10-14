#include <stdio.h>
#include <stdlib.h>
#include <time.h>
// Implementação multiplataforma para medição precisa de tempo
#ifdef _WIN32
#include <windows.h>

double get_time() {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);    // Frequência do contador de alta resolução
    QueryPerformanceCounter(&counter);   // Valor atual do contador
    return (double)counter.QuadPart / freq.QuadPart;  // Tempo em segundos
}
#else
#include <sys/time.h>
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);              // Obtém tempo atual do sistema
    return tv.tv_sec + tv.tv_usec * 1e-6; // Converte para segundos com precisão de microssegundos
}
#endif

#define N 100000000  // Tamanho grande o suficiente para medir diferenças de performance

int main() {
    double start, end;  // Variáveis para medição de tempo de execução
    // Alocação dinâmica para garantir que o vetor não interfira com otimizações de compilação
    double *vector = (double*)malloc(N * sizeof(double));
    if (!vector) {
        printf("Memory allocation failed!\n");
        return 1;
    }

    // 1) Inicialização simples do vetor - SEM dependências entre iterações
    // Permite vetorização SIMD e paralelismo completo ao nível de instrução
    start = get_time();
    for (int i = 0; i < N; i++) {
        vector[i] = i * 0.5 + 1.0;  // Cada iteração é completamente independente
    }
    end = get_time();
    printf("[1] Inicialização simples: %.6f s\n", end - start);

    // 2) Soma acumulativa - COM dependência RAW (Read After Write)
    // Cada iteração depende do resultado da anterior, limitando ILP
    double sum_sequential = 0.0;
    start = get_time();
    for (int i = 0; i < N; i++) {
        sum_sequential += vector[i];  // Dependência: precisa do valor anterior de sum_sequential
    }
    end = get_time();
    printf("[2] Soma acumulativa: %.6f s\n", end - start);

    // 3) Técnica de quebra de dependência - múltiplos acumuladores
    // Loop unrolling manual + acumuladores independentes = máximo ILP
    double sum0 = 0.0, sum1 = 0.0, sum2 = 0.0, sum3 = 0.0;  // 4 acumuladores paralelos
    start = get_time();
    int i;
    for (i = 0; i <= N - 4; i += 4) {  // Processa 4 elementos por iteração
        sum0 += vector[i];      // Cada acumulador é independente dos outros
        sum1 += vector[i+1];    // Permite execução paralela das 4 operações
        sum2 += vector[i+2];    // Compilador pode usar múltiplas unidades funcionais
        sum3 += vector[i+3];    // Facilita vetorização e pipeline superescalar
    }
    // Processa elementos restantes quando N não é múltiplo de 4
    for (; i < N; i++) sum0 += vector[i];
    double sum_parallel = sum0 + sum1 + sum2 + sum3;  // Combinação final dos resultados
    end = get_time();
    printf("[3] Soma com múltiplas variáveis: %.6f s\n ---------------------------------------------\n", end - start);

    // Uso dos resultados para evitar que o compilador otimize e remova os loops
    // Condição impossível garante que os valores sejam "usados" sem afetar medições
    if (sum_sequential == 0.999999 || sum_parallel == 0.999999)
        printf("Dummy: %.2f %.2f\n", sum_sequential, sum_parallel);

    free(vector);  // Liberação da memória alocada
    return 0;
}
