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
    unsigned int seed = 12345;  // Semente para geração de números aleatórios
    
    for (long i = 0; i < num_pontos; i++) {
        // Gerar coordenadas aleatórias no intervalo [-1, 1]
        double x = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0; // [-1, 1]
        double y = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0; // [-1, 1]
        
        // Verificar se o ponto está dentro do círculo unitário
        // Equação do círculo: x² + y² ≤ 1
        if (x*x + y*y <= 1.0) {
            pontos_dentro++;
        }
    }
    
    // Estimativa de π usando a fórmula: π ≈ 4 × (pontos_dentro / total)
    return 4.0 * pontos_dentro / num_pontos;
}

// 2. Versão INCORRETA - com condição de corrida usando #pragma omp parallel for
double estimar_pi_incorreto(long num_pontos) {
    long pontos_dentro = 0; // PROBLEMA: Variável compartilhada sem proteção!
    
    #pragma omp parallel for
    for (long i = 0; i < num_pontos; i++) {
        // Cada thread precisa de sua própria semente para evitar correlação
        unsigned int seed = i + omp_get_thread_num() * 12345;
        double x = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
        double y = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
        
        if (x*x + y*y <= 1.0) {
            pontos_dentro++; // CONDIÇÃO DE CORRIDA! Múltiplas threads modificam simultaneamente
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
            #pragma omp critical  // GARGALO: Serializa o acesso a cada incremento
            pontos_dentro++; // Agora protegido, mas com overhead de sincronização
        }
    }
    
    return 4.0 * pontos_dentro / num_pontos;
}

// 4. Reestruturação com #pragma omp parallel seguido de #pragma omp for
double estimar_pi_reestruturado(long num_pontos) {
    long pontos_dentro = 0;
    
    #pragma omp parallel  // Cria região paralela SEPARADA do loop
    {
        // Variável LOCAL para cada thread - sem compartilhamento!
        long pontos_locais = 0;
        unsigned int seed = 12345 + omp_get_thread_num() * 1000;
        
        #pragma omp for  // Distribui iterações entre threads existentes
        for (long i = 0; i < num_pontos; i++) {
            double x = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
            double y = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
            
            if (x*x + y*y <= 1.0) {
                pontos_locais++; // RÁPIDO: sem sincronização no loop!
            }
        }
        
        // Sincronização acontece apenas UMA vez por thread (4 vezes total)
        #pragma omp critical
        pontos_dentro += pontos_locais; // Acumula resultado local no total
    }
    
    return 4.0 * pontos_dentro / num_pontos;
}

// 5. Demonstração com private
double estimar_pi_private(long num_pontos) {
    long pontos_dentro = 0;
    long pontos_locais = 999; // Valor inicial que será perdido nas threads
    int thread_id = -1;       // Também será perdido
    
    printf("\n=== CLÁUSULA: PRIVATE ===\n");
    printf("Antes: pontos_locais=%ld, thread_id=%d (serão perdidos)\n", pontos_locais, thread_id);
    
    #pragma omp parallel private(pontos_locais, thread_id)
    {
        // IMPORTANTE: Valores iniciais são INDEFINIDOS em private!
        // Precisamos inicializar manualmente dentro da região paralela
        thread_id = omp_get_thread_num();
        pontos_locais = 0; // Inicialização manual obrigatória
        
        unsigned int seed = 12345 + thread_id * 1000;
        
        #pragma omp for
        for (long i = 0; i < num_pontos; i++) {
            double x = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
            double y = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
            
            if (x*x + y*y <= 1.0) {
                pontos_locais++; // Cada thread incrementa sua própria cópia
            }
        }
        
        #pragma omp critical
        {
            printf("Thread %d: %ld pontos\n", thread_id, pontos_locais);
            pontos_dentro += pontos_locais;
        }
    }
    
    printf("Depois: pontos_locais=%ld, thread_id=%d (valores originais inalterados)\n", pontos_locais, thread_id);
    
    return 4.0 * pontos_dentro / num_pontos;
}

// 6. Demonstração com firstprivate
double estimar_pi_firstprivate(long num_pontos) {
    long pontos_dentro = 0;
    long contador_inicial = 1000; // Este valor será COPIADO para cada thread
    int multiplicador = 100;      // Este também será copiado
    
    printf("\n=== CLÁUSULA: FIRSTPRIVATE ===\n");
    printf("Antes: contador_inicial=%ld, multiplicador=%d (serão copiados)\n", contador_inicial, multiplicador);
    
    #pragma omp parallel firstprivate(contador_inicial, multiplicador)
    {
        int thread_id = omp_get_thread_num();
        // Cada thread automaticamente RECEBE uma CÓPIA dos valores originais!
        
        long pontos_locais = 0;
        // Usar os valores iniciais para criar sementes diferentes por thread
        unsigned int seed = contador_inicial + thread_id * multiplicador;
        
        #pragma omp for
        for (long i = 0; i < num_pontos; i++) {
            double x = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
            double y = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
            
            if (x*x + y*y <= 1.0) {
                pontos_locais++;
            }
        }
        
        // Demonstrar que cada thread pode modificar suas cópias independentemente
        contador_inicial += pontos_locais;   // Modificação local (não afeta original)
        multiplicador *= thread_id + 1;      // Modificação local (não afeta original)
        
        #pragma omp critical
        {
            printf("Thread %d: contador=%ld, mult=%d, pontos=%ld\n", 
                   thread_id, contador_inicial, multiplicador, pontos_locais);
            pontos_dentro += pontos_locais;
        }
    }
    
    printf("Depois: contador_inicial=%ld, multiplicador=%d (valores originais preservados)\n", contador_inicial, multiplicador);
    
    return 4.0 * pontos_dentro / num_pontos;
}

// 7. Demonstração com shared
double estimar_pi_shared(long num_pontos) {
    long pontos_dentro = 0;
    long contador_compartilhado = 0; // Variável compartilhada - todas threads acessam
    double progresso = 0.0;          // Também compartilhada
    
    printf("\n=== CLÁUSULA: SHARED ===\n");
    printf("Variáveis compartilhadas: contador=%ld, progresso=%.1f%%\n", contador_compartilhado, progresso * 100);
    
    #pragma omp parallel shared(pontos_dentro, contador_compartilhado, progresso, num_pontos)
    {
        int thread_id = omp_get_thread_num();
        long pontos_locais = 0; // Esta é automática private (declarada dentro)
        unsigned int seed = 12345 + thread_id * 1000;
        
        #pragma omp for
        for (long i = 0; i < num_pontos; i++) {
            double x = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
            double y = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
            
            if (x*x + y*y <= 1.0) {
                pontos_locais++;
            }
            
            // Demonstrar acesso sincronizado a variáveis compartilhadas
            #pragma omp atomic  // Protege o incremento
            contador_compartilhado++;
            
            // Atualizar progresso ocasionalmente
            if (i % 10000 == 0) {
                #pragma omp atomic write
                progresso = (double)contador_compartilhado / num_pontos;
            }
        }
        
        #pragma omp critical
        {
            printf("Thread %d: %ld pontos\n", thread_id, pontos_locais);
            pontos_dentro += pontos_locais;
        }
    }
    
    printf("Final: contador=%ld, progresso=%.1f%% (modificadas por todas threads)\n", 
           contador_compartilhado, progresso * 100);
    
    return 4.0 * pontos_dentro / num_pontos;
}

// 8. Demonstração com lastprivate
double estimar_pi_lastprivate(long num_pontos) {
    long pontos_dentro = 0;
    int ultimo_indice = -1;           // Será sobrescrito com índice da última iteração
    int thread_da_ultima_iteracao = -1; // Será sobrescrito com ID da thread que executou por último
    
    printf("\n=== CLÁUSULA: LASTPRIVATE ===\n");
    printf("Antes: ultimo_indice=%d, thread_da_ultima_iteracao=%d\n", ultimo_indice, thread_da_ultima_iteracao);
    
    #pragma omp parallel
    {
        long pontos_locais = 0;
        int thread_id = omp_get_thread_num();
        unsigned int seed = 12345 + thread_id * 1000;
        
        // lastprivate: cada thread tem cópia própria, mas valor da última iteração é preservado
        #pragma omp for lastprivate(ultimo_indice, thread_da_ultima_iteracao)
        for (long i = 0; i < num_pontos; i++) {
            double x = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
            double y = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
            
            if (x*x + y*y <= 1.0) {
                pontos_locais++;
            }
            
            // Estas atribuições acontecem em CADA iteração
            // Mas apenas os valores da ÚLTIMA iteração serão preservados
            ultimo_indice = i;
            thread_da_ultima_iteracao = thread_id;
        }
        
        #pragma omp critical
        pontos_dentro += pontos_locais;
    }
    
    printf("Depois: ultimo_indice=%d, thread_da_ultima_iteracao=%d (valores da última iteração)\n", 
           ultimo_indice, thread_da_ultima_iteracao);
    
    return 4.0 * pontos_dentro / num_pontos;
}

// Função auxiliar para testar e medir tempo
void testar_implementacao(const char* nome, double (*funcao)(long), long num_pontos) {
    printf("\n========================================\n");
    printf("TESTANDO: %s\n", nome);
    printf("========================================\n");
    
    // Medição precisa de tempo usando OpenMP
    double tempo_inicio = omp_get_wtime(); // Timestamp de início
    double pi_estimado = funcao(num_pontos); // Execução da função
    double tempo_fim = omp_get_wtime();     // Timestamp de fim
    
    // Cálculo de métricas de qualidade
    double erro = fabs(pi_estimado - M_PI);        // Erro absoluto
    double erro_percentual = (erro / M_PI) * 100.0; // Erro percentual
    
    printf("RESULTADOS:\n");
    printf("π estimado: %.6f\n", pi_estimado);
    printf("π real:     %.6f\n", M_PI);
    printf("Erro:       %.6f (%.3f%%)\n", erro, erro_percentual);
    printf("Tempo:      %.4f segundos\n", tempo_fim - tempo_inicio);
}

int main() {
    // Configurar o número de threads para 4
    omp_set_num_threads(4);
    
    long num_pontos = 250000000; // 250 milhões de pontos para demonstração com formatação limpa
    
    printf("=== ESTIMATIVA DE π USANDO MÉTODO DE MONTE CARLO ===\n");
    printf("Número de pontos: %ld\n", num_pontos);
    printf("Número de threads configuradas: %d\n", omp_get_max_threads());
    printf("Valor real de π: %.10f\n", M_PI);
    
    // 1. Versão sequencial (referência)
    testar_implementacao("VERSÃO SEQUENCIAL", estimar_pi_sequencial, num_pontos);
    
    // 2. Demonstrar o PROBLEMA - múltiplas execuções mostram inconsistência
    printf("\n*** PROBLEMA: CONDIÇÃO DE CORRIDA COM #pragma omp parallel for ***\n");
    for (int i = 0; i < 3; i++) {
        printf("\n--- Execução %d ---\n", i + 1);
        testar_implementacao("PARALLEL FOR INCORRETO", estimar_pi_incorreto, num_pontos);
    }
    
    // 3. Primeira correção (funcional mas ineficiente)
    testar_implementacao("CORREÇÃO COM CRITICAL", estimar_pi_critical, num_pontos);
    
    // 4. Solução otimizada (melhor prática)
    testar_implementacao("REESTRUTURADO (parallel + for)", estimar_pi_reestruturado, num_pontos);
    
    // 5. Demonstrações das cláusulas
    printf("\n\n*** DEMONSTRAÇÕES DAS CLÁUSULAS OpenMP ***\n");
    
    // Usar dataset completo para demonstrações com 250 milhões de pontos
    testar_implementacao("CLÁUSULA PRIVATE", estimar_pi_private, num_pontos);
    
    testar_implementacao("CLÁUSULA FIRSTPRIVATE", estimar_pi_firstprivate, num_pontos);
    
    testar_implementacao("CLÁUSULA SHARED", estimar_pi_shared, num_pontos);
    
    testar_implementacao("CLÁUSULA LASTPRIVATE", estimar_pi_lastprivate, num_pontos);
    
    return 0;
}
