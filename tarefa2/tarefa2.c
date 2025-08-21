#include <stdio.h>
#include <stdlib.h>
#include <time.h>
// Pega o tempo atual com base no sistema operacional
#ifdef _WIN32
#include <windows.h>

double get_time() {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / freq.QuadPart;
}
#else
#include <sys/time.h>
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}
#endif

#define N 100000000

int main() {
    double start, end;
    // Aloca vetor de N elementos do tipo double
    double *vector = (double*)malloc(N * sizeof(double));
    if (!vector) {
        printf("Memory allocation failed!\n");
        return 1;
    }

    // 1) Inicialização simples do vetor
    // Cada elemento recebe: índice * 0.5 + 1.0
    start = get_time();
    for (int i = 0; i < N; i++) {
        vector[i] = i * 0.5 + 1.0;
    }
    end = get_time();
    printf("[1] Inicialização simples: %.6f s\n", end - start);

    // 2) Soma acumulativa de todos os elementos do vetor
    // Dependência entre iterações (acumulador único)
    double sum_sequential = 0.0;
    start = get_time();
    for (int i = 0; i < N; i++) {
        sum_sequential += vector[i];
    }
    end = get_time();
    printf("[2] Soma acumulativa: %.6f s\n", end - start);

    // 3) Soma com múltiplos acumuladores para quebrar dependência
    // Usa 4 acumuladores para permitir paralelismo
    double sum0 = 0.0, sum1 = 0.0, sum2 = 0.0, sum3 = 0.0;
    start = get_time();
    int i;
    for (i = 0; i <= N - 4; i += 4) {
        sum0 += vector[i];
        sum1 += vector[i+1];
        sum2 += vector[i+2];
        sum3 += vector[i+3];
    }
    // Soma o restante dos elementos (caso N não seja múltiplo de 4)
    for (; i < N; i++) sum0 += vector[i];
    double sum_parallel = sum0 + sum1 + sum2 + sum3;
    end = get_time();
    printf("[3] Soma com múltiplas variáveis: %.6f s\n ---------------------------------------------\n", end - start);

    // Referência as somas para evitar otimização dos loops e descarte dos valores
    if (sum_sequential == 0.999999 || sum_parallel == 0.999999)
        printf("Dummy: %.2f %.2f\n", sum_sequential, sum_parallel);

    free(vector);
    return 0;
}

//Comandos para compilar e executar (Linux e Windows)
/*
Linux:
gcc tarefa2.c -O0 -o tarefa2_O0
gcc tarefa2.c -O2 -o tarefa2_O2
gcc tarefa2.c -O3 -o tarefa2_O3

./tarefa2_O0
./tarefa2_O2
./tarefa2_O3

Windows (MinGW):
gcc tarefa2.c -O0 -o tarefa2_O0.exe
gcc tarefa2.c -O2 -o tarefa2_O2.exe
gcc tarefa2.c -O3 -o tarefa2_O3.exe

tarefa2_O0.exe
tarefa2_O2.exe
tarefa2_O3.exe
*/