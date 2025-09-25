/*
 * ============================================================================
 * SIMULAÇÃO NAVIER-STOKES COM OPENMP E INSTRUMENTAÇÃO PASCAL
 * ============================================================================
 * 
 * Este programa implementa uma simulação simplificada da equação de Navier-Stokes
 * para análise de escalabilidade com diferentes números de cores e estratégias
 * de paralelização (schedule static vs collapse).
 * 
 * COMPILAÇÃO:
 * -----------
 * 
 * 1. SEM PaScal (compilação básica):
 *    gcc -O2 -fopenmp tarefa11_simples.c -o tarefa11_simples -lm
 * 
 * 2. COM PaScal (instrumentação manual):
 *    gcc -O2 -fopenmp -DUSE_PASCAL tarefa11_simples.c -o tarefa11_simples -lm -lmpascalops
 * 
 * EXECUÇÃO:
 * ---------
 * 
 * 1. Execução básica:
 *    ./tarefa11_simples [grid_size] [iterations]
 *    Exemplo: ./tarefa11_simples 1024 3000
 * 
 * 2. Análise com PaScal Analyzer:
 *    pascalanalyzer ./tarefa11_simples --inst man --cors 2,4,8 --ipts "1024 3000","2048 6000" --verb INFO
 * 
 * REGIÕES DE INSTRUMENTAÇÃO PASCAL:
 * ---------------------------------
 * 
 * Região 100: Programa completo
 * Região 1:   Simulação serial completa
 * Região 11:  Loop principal serial
 * Região 12:  Cópia de dados serial
 * Região 2:   Simulação paralela static completa  
 * Região 21:  Loop principal paralelo static
 * Região 22:  Cópia de dados paralela static
 * Região 3:   Simulação paralela collapse completa
 * Região 31:  Loop principal paralelo collapse
 * Região 32:  Cópia de dados paralela collapse
 * 
 * PARÂMETROS:
 * -----------
 * - N: Tamanho da grade (NxN pontos)
 * - ITER: Número de iterações temporais
 * - DT: Passo temporal (0.00001 para estabilidade)
 * - NU: Viscosidade cinemática (0.1)
 * 
 * ALGORITMO:
 * ----------
 * - Implementa apenas a parte de viscosidade da equação de Navier-Stokes
 * - Usa diferenças finitas para aproximar o Laplaciano
 * - Condições de contorno: velocidade zero nas bordas
 * - Perturbação inicial: distribuição gaussiana no centro
 * 
 * ANÁLISE DE ESCALABILIDADE:
 * --------------------------
 * - Compara performance serial vs paralela
 * - Testa 2, 4 e 8 cores
 * - Analisa schedule static vs collapse
 * - Calcula speedup e eficiência
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

// Incluir PaScal apenas se disponível
#ifdef USE_PASCAL
#include "pascalops.h"
#define PASCAL_START(id) pascal_start(id) // Macro para iniciar medição de região
#define PASCAL_STOP(id) pascal_stop(id)   // Macro para parar medição de região
#else
// Macros vazias quando PaScal não está disponível (compilação condicional)
#define PASCAL_START(id) do {} while(0)
#define PASCAL_STOP(id) do {} while(0)
#endif

// Parâmetros da simulação (variáveis via linha de comando)
int N = 1024;        // Grade NxN pontos (padrão robusto para análise significativa)
int ITER = 3000;     // Número de iterações (padrão robusto para análise significativa)
double DT = 0.00001; // Passo temporal (pequeno para estabilidade numérica)
double NU = 0.1;     // Viscosidade cinemática (constante do fluido)

// Arrays dinâmicos para campos de velocidade
double **u, **v;         // Campos de velocidade atuais (u=horizontal, v=vertical)
double **u_new, **v_new; // Campos para próximo passo temporal (double buffering)

// Função para alocar matriz 2D dinamicamente
double** allocate_matrix(int rows, int cols) {
    double **matrix = (double**)malloc(rows * sizeof(double*)); // Aloca array de ponteiros para linhas
    for (int i = 0; i < rows; i++) {
        matrix[i] = (double*)calloc(cols, sizeof(double)); // Aloca e inicializa cada linha com zeros
    }
    return matrix;
}

// Função para liberar matriz 2D
void free_matrix(double **matrix, int rows) {
    for (int i = 0; i < rows; i++) {
        free(matrix[i]); // Libera cada linha individual
    }
    free(matrix); // Libera o array de ponteiros
}

// Função para calcular Laplaciano 2D (aproximação de diferenças finitas)
double laplacian(double **field, int i, int j) {
    // Aproximação do operador Laplaciano usando diferenças finitas centrais
    return field[i+1][j] + field[i-1][j] + field[i][j+1] + field[i][j-1] - 4.0 * field[i][j];
}

// Função para inicializar condições de contorno
void apply_boundary_conditions() {
    // Condições de contorno: velocidade zero nas bordas
    for (int i = 0; i < N; i++) {
        u[i][0] = u[i][N-1] = 0.0;
        v[i][0] = v[i][N-1] = 0.0;
        u[0][i] = u[N-1][i] = 0.0;
        v[0][i] = v[N-1][i] = 0.0;
    }
}

// Função para criar perturbação inicial
void create_perturbation() {
    int center_x = N/2, center_y = N/2; // Centro da grade
    int radius = N/8; // Raio da perturbação (1/8 do tamanho da grade)
    
    for (int i = center_x - radius; i <= center_x + radius; i++) {
        for (int j = center_y - radius; j <= center_y + radius; j++) {
            if (i >= 0 && i < N && j >= 0 && j < N) { // Verificar limites da grade
                double dx = i - center_x;
                double dy = j - center_y;
                double r = sqrt(dx*dx + dy*dy); // Distância do centro
                if (r <= radius) {
                    // Criar distribuição gaussiana para velocidade inicial
                    u[i][j] = 0.5 * exp(-(r*r)/(radius*radius/4));
                    v[i][j] = 0.3 * exp(-(r*r)/(radius*radius/4));
                }
            }
        }
    }
}

// Simulação serial (referência)
double simulate_serial() {
    printf("🔄 Executando versão SERIAL...\n");
    
    PASCAL_START(1); // Região 1: Simulação serial completa
    double start = omp_get_wtime(); // Marca tempo inicial
    
    for (int iter = 0; iter < ITER; iter++) {
        PASCAL_START(11); // Região 11: Loop principal serial
        
        // Atualizar campos de velocidade (equação da viscosidade)
        for (int i = 1; i < N-1; i++) { // Evita bordas (i=0 e i=N-1)
            for (int j = 1; j < N-1; j++) { // Evita bordas (j=0 e j=N-1)
                // Equação de difusão: u_novo = u_atual + dt * nu * laplaciano(u)
                u_new[i][j] = u[i][j] + DT * NU * laplacian(u, i, j);
                v_new[i][j] = v[i][j] + DT * NU * laplacian(v, i, j);
            }
        }
        
        PASCAL_STOP(11); // Fim do loop principal serial
        
        PASCAL_START(12); // Região 12: Cópia serial
        // Copiar valores calculados de volta para arrays principais
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                u[i][j] = u_new[i][j];
                v[i][j] = v_new[i][j];
            }
        }
        PASCAL_STOP(12); // Fim da cópia serial
        
        apply_boundary_conditions(); // Reaplicar condições de contorno
    }
    
    double end = omp_get_wtime(); // Marca tempo final
    PASCAL_STOP(1); // Fim da simulação serial completa
    
    double tempo = end - start;
    
    printf("   ⏱️  Tempo serial: %.4f segundos\n", tempo);
    printf("   🔄 %.1f iterações/segundo\n", ITER / tempo);
    
    return tempo; // Retorna tempo de execução para análise de escalabilidade
}

// Simulação paralela com schedule estático
double simulate_parallel_static(int num_threads) {
    printf("🚀 Executando versão PARALELA (schedule static, %d threads)...\n", num_threads);
    
    PASCAL_START(2); // Região 2: Simulação paralela static completa
    
    omp_set_num_threads(num_threads); // Define número de threads OpenMP
    double start = omp_get_wtime(); // Marca tempo inicial
    
    for (int iter = 0; iter < ITER; iter++) {
        PASCAL_START(21); // Região 21: Loop principal paralelo static
        
        // Paralelização do loop principal com distribuição estática de trabalho
        #pragma omp parallel for schedule(static)
        for (int i = 1; i < N-1; i++) {
            for (int j = 1; j < N-1; j++) {
                u_new[i][j] = u[i][j] + DT * NU * laplacian(u, i, j);
                v_new[i][j] = v[i][j] + DT * NU * laplacian(v, i, j);
            }
        }
        
        PASCAL_STOP(21); // Fim do loop principal paralelo static
        
        PASCAL_START(22); // Região 22: Cópia paralela static
        // Paralelização da cópia com distribuição dinâmica padrão
        #pragma omp parallel for
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                u[i][j] = u_new[i][j];
                v[i][j] = v_new[i][j];
            }
        }
        PASCAL_STOP(22); // Fim da cópia paralela static
        
        apply_boundary_conditions(); // Reaplicar condições de contorno
    }
    
    double end = omp_get_wtime(); // Marca tempo final
    PASCAL_STOP(2); // Fim da simulação paralela static completa
    
    double tempo = end - start;
    
    printf("   ⏱️  Tempo paralelo: %.4f segundos\n", tempo);
    printf("   🔄 %.1f iterações/segundo\n", ITER / tempo);
    
    return tempo; // Retorna tempo de execução para análise de escalabilidade
}

// Simulação paralela com collapse
double simulate_parallel_collapse(int num_threads) {
    printf("🚀 Executando versão PARALELA (collapse, %d threads)...\n", num_threads);
    
    PASCAL_START(3); // Região 3: Simulação paralela collapse completa
    
    omp_set_num_threads(num_threads); // Define número de threads OpenMP
    double start = omp_get_wtime(); // Marca tempo inicial
    
    for (int iter = 0; iter < ITER; iter++) {
        PASCAL_START(31); // Região 31: Loop principal paralelo collapse
        
        // Paralelização com collapse - combina 2 loops em um espaço de iteração único
        #pragma omp parallel for collapse(2)
        for (int i = 1; i < N-1; i++) {
            for (int j = 1; j < N-1; j++) {
                u_new[i][j] = u[i][j] + DT * NU * laplacian(u, i, j);
                v_new[i][j] = v[i][j] + DT * NU * laplacian(v, i, j);
            }
        }
        
        PASCAL_STOP(31); // Fim do loop principal paralelo collapse
        
        PASCAL_START(32); // Região 32: Cópia paralela collapse
        // Paralelização da cópia também com collapse para melhor distribuição
        #pragma omp parallel for collapse(2)
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                u[i][j] = u_new[i][j];
                v[i][j] = v_new[i][j];
            }
        }
        PASCAL_STOP(32); // Fim da cópia paralela collapse
        
        apply_boundary_conditions(); // Reaplicar condições de contorno
    }
    
    double end = omp_get_wtime(); // Marca tempo final
    PASCAL_STOP(3); // Fim da simulação paralela collapse completa
    
    double tempo = end - start;
    
    printf("   ⏱️  Tempo paralelo: %.4f segundos\n", tempo);
    printf("   🔄 %.1f iterações/segundos\n", ITER / tempo);
    
    return tempo; // Retorna tempo de execução para análise de escalabilidade
}

int main(int argc, char *argv[]) {
    // Processar argumentos da linha de comando
    if (argc >= 2) N = atoi(argv[1]); // Primeiro argumento: tamanho da grade
    if (argc >= 3) ITER = atoi(argv[2]); // Segundo argumento: número de iterações
    
    printf("╔═══════════════════════════════════════════════════════════════════╗\n");
    printf("║           🌊 SIMULAÇÃO NAVIER-STOKES COM OPENMP 🌊              ║\n");
    printf("║                    Análise de Escalabilidade                     ║\n");
    #ifdef USE_PASCAL
    printf("║                 📊 INSTRUMENTADO COM PASCAL 📊                   ║\n");
    #endif
    printf("╠═══════════════════════════════════════════════════════════════════╣\n");
    printf("║ 📏 Grid: %dx%d pontos                                          ║\n", N, N);
    printf("║ 🔄 Iterações: %d                                               ║\n", ITER);
    printf("║ ⚡ Threads disponíveis: %d                                      ║\n", omp_get_max_threads());
    printf("╚═══════════════════════════════════════════════════════════════════╝\n\n");
    
    #ifdef USE_PASCAL
    printf("📊 REGIÕES DE INSTRUMENTAÇÃO PASCAL:\n");
    printf("   Região 1:  Simulação serial completa\n");
    printf("   Região 11: Loop principal serial\n");
    printf("   Região 12: Cópia de dados serial\n");
    printf("   Região 2:  Simulação paralela static completa\n");
    printf("   Região 21: Loop principal paralelo static\n");
    printf("   Região 22: Cópia de dados paralela static\n");
    printf("   Região 3:  Simulação paralela collapse completa\n");
    printf("   Região 31: Loop principal paralelo collapse\n");
    printf("   Região 32: Cópia de dados paralela collapse\n\n");
    #endif
    
    PASCAL_START(100); // Região 100: Programa completo
    
    // Alocar matrizes dinamicamente para os campos de velocidade
    u = allocate_matrix(N, N);     // Campo de velocidade u atual
    v = allocate_matrix(N, N);     // Campo de velocidade v atual
    u_new = allocate_matrix(N, N); // Campo de velocidade u para próximo passo
    v_new = allocate_matrix(N, N); // Campo de velocidade v para próximo passo
    
    // Aplicar condições iniciais
    create_perturbation();    // Criar perturbação gaussiana no centro
    apply_boundary_conditions(); // Aplicar condições de contorno (velocidade zero nas bordas)
    
    // Array para armazenar tempos de execução
    double tempo_serial;
    double tempos_paralelos[3]; // Para 2, 4 e 8 threads
    int cores[] = {2, 4, 8};    // Configurações de threads a testar
    
    // TESTE 1: Versão Serial
    printf("═══════════════════════════════════════════════════════════════════\n");
    tempo_serial = simulate_serial();
    
    // TESTE 2: Versões Paralelas (schedule static)
    printf("\n═══════════════════════════════════════════════════════════════════\n");
    printf("                    TESTE SCHEDULE STATIC\n");
    printf("═══════════════════════════════════════════════════════════════════\n");
    
    for (int i = 0; i < 3; i++) {
        // Resetar condições para cada teste para garantir comparação justa
        create_perturbation();
        apply_boundary_conditions();
        
        tempos_paralelos[i] = simulate_parallel_static(cores[i]);
        printf("\n");
    }
    
    // TESTE 3: Versões Paralelas (collapse)
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf("                     TESTE COLLAPSE\n");
    printf("═══════════════════════════════════════════════════════════════════\n");
    
    double tempos_collapse[3]; // Armazenar tempos das execuções collapse
    for (int i = 0; i < 3; i++) {
        // Resetar condições para cada teste para garantir comparação justa
        create_perturbation();
        apply_boundary_conditions();
        
        tempos_collapse[i] = simulate_parallel_collapse(cores[i]);
        printf("\n");
    }
    
    // ANÁLISE DE RESULTADOS
    printf("╔═══════════════════════════════════════════════════════════════════╗\n");
    printf("║                    📊 ANÁLISE DE ESCALABILIDADE                  ║\n");
    printf("╠═══════════════════════════════════════════════════════════════════╣\n");
    printf("║ Tempo Serial: %.4f segundos                                    ║\n", tempo_serial);
    printf("╠═══════════════════════════════════════════════════════════════════╣\n");
    printf("║                       SCHEDULE STATIC                            ║\n");
    for (int i = 0; i < 3; i++) {
        double speedup = tempo_serial / tempos_paralelos[i]; // Calcular aceleração
        double eficiencia = speedup / cores[i] * 100;       // Calcular eficiência percentual
        printf("║ %d cores: %.4fs (speedup: %.2fx, eficiência: %.1f%%)           ║\n", 
               cores[i], tempos_paralelos[i], speedup, eficiencia);
    }
    printf("╠═══════════════════════════════════════════════════════════════════╣\n");
    printf("║                         COLLAPSE                                 ║\n");
    for (int i = 0; i < 3; i++) {
        double speedup = tempo_serial / tempos_collapse[i]; // Calcular aceleração
        double eficiencia = speedup / cores[i] * 100;      // Calcular eficiência percentual
        printf("║ %d cores: %.4fs (speedup: %.2fx, eficiência: %.1f%%)           ║\n", 
               cores[i], tempos_collapse[i], speedup, eficiencia);
    }
    printf("╚═══════════════════════════════════════════════════════════════════╝\n");
    
    PASCAL_STOP(100); // Fim do programa completo
    
    // Liberar memória alocada dinamicamente
    free_matrix(u, N);
    free_matrix(v, N);
    free_matrix(u_new, N);
    free_matrix(v_new, N);
    
    #ifdef USE_PASCAL
    printf("\n📁 Dados PaScal coletados para análise de escalabilidade.\n");
    printf("💡 Use pascalanalyzer para análise automática:\n");
    printf("   pascalanalyzer ./tarefa11_simples --inst man --cors 2,4,8 --ipts \"%d %d\" --verb INFO\n", N, ITER);
    #endif
    
    printf("\n✨ Análise de escalabilidade concluída! ✨\n");
    
    return 0;
}
//export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/pascal-releases-master/lib && ./pascal-releases-master/bin/pascalanalyzer ./tarefa11_simples_pascal --inst man --cors 2,4,6,8 --ipts "512 1500","1024 3000","2048 6000" --outp pascalanalysis.json --verb INFO
//export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/pascal-releases-master/lib && ./pascal-releases-master/bin/pascalanalyzer ./tarefa11_simples_pascal --inst man --cors 2,4,8 --ipts "1024 3000","2048 6000","4096 12000" --rpts 2 --outp pascal_analysis.json --verb INFO
