#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

int main(int argc, char *argv[]) {
    long long int N = 100000000; // Número de pontos para Monte Carlo
    int nthreads = 4; // Número de threads para paralelização
    if (argc > 1) N = atoll(argv[1]); // Aceita N como argumento
    if (argc > 2) nthreads = atoi(argv[2]); // Aceita número de threads como argumento

    // 1. Compartilhado + critical
    long long int acertos_critical = 0;
    double start = omp_get_wtime();
    #pragma omp parallel num_threads(nthreads)
    {
        unsigned int seed = (unsigned int)time(NULL) ^ omp_get_thread_num(); // Seed única por thread
        #pragma omp for
        for (long long int i = 0; i < N; i++) {
            double x = (double)rand_r(&seed) / RAND_MAX; // Números aleatórios thread-safe
            double y = (double)rand_r(&seed) / RAND_MAX;
            if (x*x + y*y <= 1.0) { // Teste se ponto está dentro do círculo
                #pragma omp critical // Serializa acesso - muito lento
                acertos_critical++;
            }
        }
    }
    double pi = 4.0 * (double)acertos_critical / (double)N; // Estimativa de π: 4 * (pontos no círculo / total)
    double end = omp_get_wtime(); // Medição precisa de tempo
    printf("Versao 1 (critical): pi = %.10f | Tempo: %.5f s\n", pi, end-start);

    // 2. Compartilhado + atomic
    long long int acertos_atomic = 0;
    start = omp_get_wtime();
    #pragma omp parallel num_threads(nthreads)
    {
        unsigned int seed = (unsigned int)time(NULL) ^ omp_get_thread_num(); // Seed única por thread
        #pragma omp for
        for (long long int i = 0; i < N; i++) {
            double x = (double)rand_r(&seed) / RAND_MAX;
            double y = (double)rand_r(&seed) / RAND_MAX;
            if (x*x + y*y <= 1.0) {
                #pragma omp atomic // Operação atômica em hardware - mais rápida que critical
                acertos_atomic++;
            }
        }
    }
    pi = 4.0 * (double)acertos_atomic / (double)N;
    end = omp_get_wtime();
    printf("Versao 2 (atomic):   pi = %.10f | Tempo: %.5f s\n", pi, end-start);

    // 3. Contador privado (redução manual)
    long long int acertos_privado = 0;
    start = omp_get_wtime();
    #pragma omp parallel num_threads(nthreads)
    {
        unsigned int seed = (unsigned int)time(NULL) ^ omp_get_thread_num(); // Seed única por thread
        long long int local = 0; // Contador local - sem contenção
        #pragma omp for
        for (long long int i = 0; i < N; i++) {
            double x = (double)rand_r(&seed) / RAND_MAX;
            double y = (double)rand_r(&seed) / RAND_MAX;
            if (x*x + y*y <= 1.0) local++; // Incremento local - máxima performance
        }
        #pragma omp atomic // Redução manual - uma sincronização por thread
        acertos_privado += local;
    }
    pi = 4.0 * (double)acertos_privado / (double)N;
    end = omp_get_wtime();
    printf("Versao 3 (privado):  pi = %.10f | Tempo: %.5f s\n", pi, end-start);

    // 4. Vetor de contadores privados
    long long int *acertos_vet = malloc(nthreads * sizeof(long long int)); // Array para resultados
    for (int i = 0; i < nthreads; i++) acertos_vet[i] = 0;
    start = omp_get_wtime();
    #pragma omp parallel num_threads(nthreads)
    {
        int tid = omp_get_thread_num(); // ID da thread
        unsigned int seed = (unsigned int)time(NULL) ^ tid; // Seed única por thread
        long long int local = 0; // Contador local
        #pragma omp for
        for (long long int i = 0; i < N; i++) {
            double x = (double)rand_r(&seed) / RAND_MAX;
            double y = (double)rand_r(&seed) / RAND_MAX;
            if (x*x + y*y <= 1.0) local++;
        }
        acertos_vet[tid] = local; // Cada thread escreve em posição única - zero contenção
    }
    long long int acertos_total = 0;
    for (int i = 0; i < nthreads; i++) acertos_total += acertos_vet[i]; // Redução sequencial
    pi = 4.0 * (double)acertos_total / (double)N;
    end = omp_get_wtime();
    printf("Versao 4 (vetor):    pi = %.10f | Tempo: %.5f s\n", pi, end-start);
    free(acertos_vet);

    // 5. Reduction
    long long int acertos_reduction = 0;
    start = omp_get_wtime();
    #pragma omp parallel num_threads(nthreads) reduction(+:acertos_reduction) // Compilador otimiza automaticamente
    {
        unsigned int seed = (unsigned int)time(NULL) ^ omp_get_thread_num(); // Seed única por thread
        #pragma omp for
        for (long long int i = 0; i < N; i++) {
            double x = (double)rand_r(&seed) / RAND_MAX;
            double y = (double)rand_r(&seed) / RAND_MAX;
            if (x*x + y*y <= 1.0) acertos_reduction++; // Melhor performance + código mais limpo
        }
    }
    pi = 4.0 * (double)acertos_reduction / (double)N;
    end = omp_get_wtime();
    printf("Versao 5 (reduction):pi = %.10f | Tempo: %.5f s\n", pi, end-start);

    return 0;
}
