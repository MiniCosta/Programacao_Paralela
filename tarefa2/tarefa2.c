#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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

#define N 1000000000

int main() {
    double start, end;
    double *v = (double*)malloc(N * sizeof(double));
    if (!v) {
        printf("Memory allocation failed!\n");
        return 1;
    }

    // 1) Inicialização simples
    start = get_time();
    for (int i = 0; i < N; i++) {
        v[i] = i * 0.5 + 1.0;
    }
    end = get_time();
    printf("[1] Inicialização simples: %.6f s\n", end - start);

    // 2) Soma acumulativa (dependência entre iterações)
    double acc = 0.0;
    start = get_time();
    for (int i = 0; i < N; i++) {
        acc += v[i];
    }
    end = get_time();
    printf("[2] Soma acumulativa: %.6f s (acc=%.2f)\n", end - start, acc);

    // 3) Soma com múltiplas variáveis (quebra dependência)
    double acc0 = 0.0, acc1 = 0.0, acc2 = 0.0, acc3 = 0.0;
    start = get_time();
    int i;
    for (i = 0; i <= N - 4; i += 4) {
        acc0 += v[i];
        acc1 += v[i+1];
        acc2 += v[i+2];
        acc3 += v[i+3];
    }
    // Resto
    for (; i < N; i++) acc0 += v[i];
    double acc_total = acc0 + acc1 + acc2 + acc3;
    end = get_time();
    printf("[3] Soma com múltiplas variáveis: %.6f s (acc=%.2f)\n", end - start, acc_total);

    free(v);
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