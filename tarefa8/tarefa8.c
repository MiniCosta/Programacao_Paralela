#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

int main(int argc, char *argv[]) {
    long long int N = 100000000; // número de pontos
    int nthreads = 4;
    if (argc > 1) N = atoll(argv[1]);
    if (argc > 2) nthreads = atoi(argv[2]);

    // Versão 1: região crítica
    long long int acertos = 0;
    double start = omp_get_wtime();
    #pragma omp parallel num_threads(nthreads)
    {
        long long int acertos_priv = 0;
        #pragma omp for
        for (long long int i = 0; i < N; i++) {
            double x = (double)rand() / RAND_MAX;
            double y = (double)rand() / RAND_MAX;
            if (x*x + y*y <= 1.0) acertos_priv++;
        }
        #pragma omp critical
        acertos += acertos_priv;
    }
    double pi = 4.0 * (double)acertos / (double)N;
    double end = omp_get_wtime();
    printf("Versão 1 (rand + critical):\n");
    printf("pi = %.10f\n", pi);
    printf("Tempo: %.5f s\n\n", end - start);

    // Versão 2: vetor compartilhado
    long long int *acertos_vet = malloc(nthreads * sizeof(long long int));
    for (int i = 0; i < nthreads; i++) acertos_vet[i] = 0;
    start = omp_get_wtime();
    #pragma omp parallel num_threads(nthreads)
    {
        int tid = omp_get_thread_num();
        long long int acertos_priv = 0;
        #pragma omp for
        for (long long int i = 0; i < N; i++) {
            double x = (double)rand() / RAND_MAX;
            double y = (double)rand() / RAND_MAX;
            if (x*x + y*y <= 1.0) acertos_priv++;
        }
        acertos_vet[tid] = acertos_priv;
    }
    long long int acertos_total = 0;
    for (int i = 0; i < nthreads; i++) acertos_total += acertos_vet[i];
    pi = 4.0 * (double)acertos_total / (double)N;
    end = omp_get_wtime();
    printf("Versão 2 (rand + vetor):\n");
    printf("pi = %.10f\n", pi);
    printf("Tempo: %.5f s\n\n", end - start);

    // Versão 3: região crítica com rand_r()
    acertos = 0;
    start = omp_get_wtime();
    #pragma omp parallel num_threads(nthreads)
    {
        unsigned int seed = (unsigned int)time(NULL) ^ omp_get_thread_num();
        long long int acertos_priv = 0;
        #pragma omp for
        for (long long int i = 0; i < N; i++) {
            double x = (double)rand_r(&seed) / RAND_MAX;
            double y = (double)rand_r(&seed) / RAND_MAX;
            if (x*x + y*y <= 1.0) acertos_priv++;
        }
        #pragma omp critical
        acertos += acertos_priv;
    }
    pi = 4.0 * (double)acertos / (double)N;
    end = omp_get_wtime();
    printf("Versão 3 (rand_r + critical):\n");
    printf("pi = %.10f\n", pi);
    printf("Tempo: %.5f s\n\n", end - start);

    // Versão 4: vetor compartilhado com rand_r()
    for (int i = 0; i < nthreads; i++) acertos_vet[i] = 0;
    start = omp_get_wtime();
    #pragma omp parallel num_threads(nthreads)
    {
        int tid = omp_get_thread_num();
        unsigned int seed = (unsigned int)time(NULL) ^ tid;
        long long int acertos_priv = 0;
        #pragma omp for
        for (long long int i = 0; i < N; i++) {
            double x = (double)rand_r(&seed) / RAND_MAX;
            double y = (double)rand_r(&seed) / RAND_MAX;
            if (x*x + y*y <= 1.0) acertos_priv++;
        }
        acertos_vet[tid] = acertos_priv;
    }
    acertos_total = 0;
    for (int i = 0; i < nthreads; i++) acertos_total += acertos_vet[i];
    pi = 4.0 * (double)acertos_total / (double)N;
    end = omp_get_wtime();
    printf("Versão 4 (rand_r + vetor):\n");
    printf("pi = %.10f\n", pi);
    printf("Tempo: %.5f s\n", end - start);

    free(acertos_vet);
    return 0;
}
