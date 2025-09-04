#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <omp.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 1. Versão sequencial (referência)
double estimar_pi_sequencial(long num_pontos) {
    long pontos_dentro = 0;
    unsigned int seed = 12345;
    
    for (long i = 0; i < num_pontos; i++) {
        double x = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0; // [-1, 1]
        double y = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0; // [-1, 1]
        
        if (x*x + y*y <= 1.0) {
            pontos_dentro++;
        }
    }
    
    return 4.0 * pontos_dentro / num_pontos;
}

// 2. Versão INCORRETA - com condição de corrida
double estimar_pi_race_condition(long num_pontos) {
    long pontos_dentro = 0; // PROBLEMA: Variável compartilhada sem proteção!
    
    #pragma omp parallel for
    for (long i = 0; i < num_pontos; i++) {
        unsigned int seed = i + omp_get_thread_num() * 12345;
        double x = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
        double y = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
        
        if (x*x + y*y <= 1.0) {
            pontos_dentro++; // CONDIÇÃO DE CORRIDA!
        }
    }
    
    return 4.0 * pontos_dentro / num_pontos;
}

// 3. Correção com #pragma omp critical
double estimar_pi_critical(long num_pontos) {
    long pontos_dentro = 0;
    
    #pragma omp parallel for
    for (long i = 0; i < num_pontos; i++) {
        unsigned int seed = i + omp_get_thread_num() * 12345;
        double x = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
        double y = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
        
        if (x*x + y*y <= 1.0) {
            #pragma omp critical
            pontos_dentro++; // Protegido contra condição de corrida
        }
    }
    
    return 4.0 * pontos_dentro / num_pontos;
}

// 4. Demonstração das cláusulas OpenMP
double estimar_pi_clausulas(long num_pontos) {
    long pontos_dentro = 0;
    int valor_inicial = 100;
    int valor_final = 0;
    int thread_id = -1;
    long contador_compartilhado = 0;
    
    #pragma omp parallel \
        private(thread_id) \
        firstprivate(valor_inicial) \
        shared(pontos_dentro, contador_compartilhado, num_pontos)
    {
        // PRIVATE: Cada thread tem sua própria cópia
        thread_id = omp_get_thread_num();
        
        // FIRSTPRIVATE: Inicializada com valor da thread mestre
        int meu_valor = valor_inicial + thread_id * 10;
        
        long pontos_locais = 0;
        
        #pragma omp for
        for (long i = 0; i < num_pontos; i++) {
            unsigned int seed = i + thread_id * 1000;
            double x = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
            double y = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
            
            if (x*x + y*y <= 1.0) {
                pontos_locais++;
            }
            
            // SHARED: Variável compartilhada entre threads
            #pragma omp atomic
            contador_compartilhado++;
        }
        
        // Acumular resultado final
        #pragma omp atomic
        pontos_dentro += pontos_locais;
        
        // Simular modificação de valor_final (só a última thread terá efeito)
        valor_final = thread_id * 999;
    }
    
    return 4.0 * pontos_dentro / num_pontos;
}

// 5. Demonstração com lastprivate
double estimar_pi_lastprivate(long num_pontos) {
    long pontos_dentro = 0;
    int ultimo_indice = -1;
    int thread_final = -1;
    
    #pragma omp parallel for \
        lastprivate(ultimo_indice, thread_final) \
        reduction(+:pontos_dentro)
    for (long i = 0; i < num_pontos; i++) {
        unsigned int seed = i + omp_get_thread_num() * 1000;
        double x = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
        double y = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
        
        if (x*x + y*y <= 1.0) {
            pontos_dentro++;
        }
        
        // LASTPRIVATE: Valor da última iteração será preservado
        ultimo_indice = i;
        thread_final = omp_get_thread_num();
    }
    
    return 4.0 * pontos_dentro / num_pontos;
}

// 6. Versão otimizada com reduction
double estimar_pi_reduction(long num_pontos) {
    long pontos_dentro = 0;
    
    #pragma omp parallel for reduction(+:pontos_dentro)
    for (long i = 0; i < num_pontos; i++) {
        unsigned int seed = i + omp_get_thread_num() * 1000;
        double x = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
        double y = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
        
        if (x*x + y*y <= 1.0) {
            pontos_dentro++;
        }
    }
    
    return 4.0 * pontos_dentro / num_pontos;
}

// Função auxiliar para testar e medir tempo
void testar_implementacao(const char* nome, double (*funcao)(long), long num_pontos) {
    double tempo_inicio = omp_get_wtime();
    double pi_estimado = funcao(num_pontos);
    double tempo_fim = omp_get_wtime();
    
    double erro = fabs(pi_estimado - M_PI);
    double erro_percentual = (erro / M_PI) * 100.0;
    
    printf("%s: π=%.6f, erro=%.3f%%, tempo=%.4fs\n", 
           nome, pi_estimado, erro_percentual, tempo_fim - tempo_inicio);
}

int main() {
    long num_pontos = 1000000; // 1 milhão de pontos
    
    printf("=== ESTIMATIVA DE π USANDO MÉTODO DE MONTE CARLO ===\n");
    printf("Número de pontos: %ld\n", num_pontos);
    printf("Threads disponíveis: %d\n", omp_get_max_threads());
    printf("Valor real de π: %.10f\n\n", M_PI);
    
    // 1. Versão sequencial (referência)
    testar_implementacao("VERSÃO SEQUENCIAL", estimar_pi_sequencial, num_pontos);
    
    // 2. Demonstrar o problema - condição de corrida
    printf("--- CONDIÇÃO DE CORRIDA ---\n");
    for (int i = 0; i < 3; i++) {
        testar_implementacao("VERSÃO COM RACE CONDITION", 
                            estimar_pi_race_condition, num_pontos / 10);
    }
    
    // 3. Correções
    printf("--- CORREÇÕES ---\n");
    testar_implementacao("VERSÃO COM CRITICAL", estimar_pi_critical, num_pontos);
    testar_implementacao("VERSÃO COM REDUCTION", estimar_pi_reduction, num_pontos);
    
    // 4. Demonstração das cláusulas
    printf("--- DEMONSTRAÇÃO DAS CLÁUSULAS ---\n");
    testar_implementacao("CLÁUSULAS OpenMP", estimar_pi_clausulas, num_pontos / 100);
    testar_implementacao("LASTPRIVATE", estimar_pi_lastprivate, num_pontos / 100);
    
    return 0;
}
