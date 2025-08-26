#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <time.h>

// Função para verificar se um número é primo
int eh_primo(int n) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    
    int limite = (int)sqrt(n);
    for (int i = 3; i <= limite; i += 2) {
        if (n % i == 0) return 0;
    }
    return 1;
}

// Versão sequencial
int contar_primos_sequencial(int n) {
    int contador = 0;
    
    for (int i = 2; i <= n; i++) {
        if (eh_primo(i)) {
            contador++;
        }
    }
    
    return contador;
}

// Versão paralela com OpenMP
int contar_primos_paralelo(int n) {
    int contador = 0;
    
    #pragma omp parallel for reduction(+:contador)
    for (int i = 2; i <= n; i++) {
        if (eh_primo(i)) {
            contador++;
        }
    }
    
    return contador;
}

// Função para medir tempo de execução
double medir_tempo(int (*funcao)(int), int n) {
    double inicio = omp_get_wtime();
    int resultado = funcao(n);
    double fim = omp_get_wtime();
    return fim - inicio;
}

int main() {
    int n;
    scanf("%d", &n);
    
    printf("\n=== CONTAGEM DE NÚMEROS PRIMOS ===\n");
    printf("Intervalo: 2 a %d\n", n);
    printf("Número de threads disponíveis: %d\n", omp_get_max_threads());
    
    // Medindo tempo da versão sequencial
    double tempo_seq = medir_tempo(contar_primos_sequencial, n);
    int primos_seq = contar_primos_sequencial(n);
    
    // Medindo tempo da versão paralela
    double tempo_par = medir_tempo(contar_primos_paralelo, n);
    int primos_par = contar_primos_paralelo(n);
    
    // Exibindo resultados
    printf("\n=== RESULTADOS ===\n");
    printf("\nTempo sequencial: %.6f segundos\n", tempo_seq);
    printf("Tempo paralelo:   %.6f segundos\n", tempo_par);
    
    if (tempo_seq > 0) {
        double speedup = tempo_seq / tempo_par;
        printf("Speedup: %.2fx\n", speedup);
        printf("Eficiência: %.2f%%\n", (speedup / omp_get_max_threads()) * 100);
    }
    
    // Verificação de correção
    if (primos_seq != primos_par) {
        printf("\n✗ ERRO! Resultados diferentes entre as versões.\n");
    } 

    return 0;
}

// gcc -fopenmp tarefa5.c -o tarefa5 -lm
// echo "1000000" | ./tarefa5