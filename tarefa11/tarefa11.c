#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include "pascalops.h"

// Parâmetros da simulação (agora variáveis)
int N = 512;         // Grade NxN pontos (pode ser alterado via linha de comando)  
int ITER = 5000;     // Número total de iterações temporais (pode ser alterado via linha de comando)
double DT = 0.00001; // Passo temporal para estabilidade numérica
double NU = 0.1;     // Viscosidade cinemática do fluido

// Arrays dinâmicos serão alocados em runtime
double **u, **v;         // Campos de velocidade atuais (u=x, v=y)
double **u_new, **v_new; // Campos de velocidade para próximo passo

// Função para alocar matriz 2D dinamicamente
double** allocate_matrix(int rows, int cols) {
    double **matrix = (double**)malloc(rows * sizeof(double*));
    for (int i = 0; i < rows; i++) {
        matrix[i] = (double*)calloc(cols, sizeof(double));
    }
    return matrix;
}

// Função para liberar matriz 2D
void free_matrix(double **matrix, int rows) {
    for (int i = 0; i < rows; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

// Função para inicializar todas as matrizes
void initialize_matrices() {
    u = allocate_matrix(N, N);
    v = allocate_matrix(N, N);
    u_new = allocate_matrix(N, N);
    v_new = allocate_matrix(N, N);
}

// Função para limpar todas as matrizes
void cleanup_matrices() {
    free_matrix(u, N);
    free_matrix(v, N);
    free_matrix(u_new, N);
    free_matrix(v_new, N);
}

// Função para resetar matrizes (zerar valores)
void reset_matrices() {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            u[i][j] = v[i][j] = 0.0;
            u_new[i][j] = v_new[i][j] = 0.0;
        }
    }
}

// Função para criar perturbação inicial
void create_perturbation() {
    int center = N/2; // Ponto central da grade
    int perturbation_size = N/20; // Tamanho da perturbação proporcional à grade
    if (perturbation_size < 3) perturbation_size = 3; // Mínimo de 3x3
    
    for (int i = center - perturbation_size; i <= center + perturbation_size; i++) {
        for (int j = center - perturbation_size; j <= center + perturbation_size; j++) {
            if (i >= 0 && i < N && j >= 0 && j < N) {
                u[i][j] = 1.0; // Velocidade inicial em x
                v[i][j] = 0.5; // Velocidade inicial em y
            }
        }
    }
}

// Calcular laplaciano usando diferenças finitas de 5 pontos
double laplacian(double **field, int i, int j) {
    if (i == 0 || i == N-1 || j == 0 || j == N-1) return 0.0; // Condições de contorno
    // ∇²f = (f[i-1,j] + f[i+1,j] + f[i,j-1] + f[i,j+1] - 4*f[i,j]) / h²
    return field[i-1][j] + field[i+1][j] + field[i][j-1] + field[i][j+1] - 4*field[i][j];
}

// VERSÃO 1: Simulação serial (baseline para comparação)
void simulate_serial() {
    printf("=== VERSÃO 1: SIMULAÇÃO SERIAL ===\n");
    double start = omp_get_wtime(); // Marcar início do tempo
    
    for (int iter = 0; iter < ITER; iter++) {
        // Loop sobre pontos internos da grade (excluindo bordas)
        for (int i = 1; i < N-1; i++) {
            for (int j = 1; j < N-1; j++) {
                // Equação de difusão: du/dt = ν * ∇²u
                u_new[i][j] = u[i][j] + DT * NU * laplacian(u, i, j);
                v_new[i][j] = v[i][j] + DT * NU * laplacian(v, i, j);
            }
        }
        
        // Copiar valores calculados para arrays principais
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                u[i][j] = u_new[i][j];
                v[i][j] = v_new[i][j];
            }
        }
    }
    
    double end = omp_get_wtime(); // Marcar fim do tempo
    printf("Tempo VERSÃO 1 (serial): %.4f segundos\n", end - start);
}

// VERSÃO 2: Simulação paralela com static schedule
void simulate_static(int threads) {
    printf("=== VERSÃO 2: SIMULAÇÃO STATIC (%d threads) ===\n", threads);
    omp_set_num_threads(threads); // Definir número de threads
    double start = omp_get_wtime();
    
    pascal_start(2); // Iniciar medição da região paralela static
    for (int iter = 0; iter < ITER; iter++) {
        // Paralelizar loop externo com divisão estática
        #pragma omp parallel for schedule(static)
        for (int i = 1; i < N-1; i++) {
            for (int j = 1; j < N-1; j++) {
                u_new[i][j] = u[i][j] + DT * NU * laplacian(u, i, j);
                v_new[i][j] = v[i][j] + DT * NU * laplacian(v, i, j);
            }
        }
        
        // Paralelizar cópia dos valores (sem schedule especificado)
        #pragma omp parallel for
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                u[i][j] = u_new[i][j];
                v[i][j] = v_new[i][j];
            }
        }
    }
    pascal_stop(2); // Finalizar medição da região paralela static
    
    double end = omp_get_wtime();
    printf("Tempo VERSÃO 2 (static): %.4f segundos\n", end - start);
}

// VERSÃO 3: Simulação com collapse (combina loops aninhados)
void simulate_collapse(int threads) {
    printf("=== VERSÃO 3: SIMULAÇÃO COLLAPSE (%d threads) ===\n", threads);
    omp_set_num_threads(threads);
    double start = omp_get_wtime();
    
    pascal_start(3); // Iniciar medição da região paralela collapse
    for (int iter = 0; iter < ITER; iter++) {
        // Collapse(2) trata os dois loops como um espaço de iteração único
        #pragma omp parallel for schedule(static) collapse(2)
        for (int i = 1; i < N-1; i++) {
            for (int j = 1; j < N-1; j++) {
                u_new[i][j] = u[i][j] + DT * NU * laplacian(u, i, j);
                v_new[i][j] = v[i][j] + DT * NU * laplacian(v, i, j);
            }
        }
        
        // Collapse(2) também na cópia para máxima paralelização
        #pragma omp parallel for collapse(2)
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                u[i][j] = u_new[i][j];
                v[i][j] = v_new[i][j];
            }
        }
    }
    pascal_stop(3); // Finalizar medição da região paralela collapse
    
    double end = omp_get_wtime();
    printf("Tempo VERSÃO 3 (collapse): %.4f segundos\n", end - start);
}

int main(int argc, char *argv[]) {
    // Processar argumentos da linha de comando
    if (argc >= 2) {
        int temp_N = atoi(argv[1]); // Primeiro argumento: tamanho da grade
        if (temp_N <= 0 || temp_N > 2048) {
            printf("Erro: Tamanho da grade deve estar entre 1 e 2048\n");
            printf("Uso: %s [tamanho_grade] [num_iteracoes]\n", argv[0]);
            printf("Exemplo: %s 512 5000\n", argv[0]);
            return 1;
        }
        N = temp_N;
    }
    
    if (argc >= 3) {
        int temp_ITER = atoi(argv[2]); // Segundo argumento: número de iterações
        if (temp_ITER <= 0 || temp_ITER > 50000) {
            printf("Erro: Número de iterações deve estar entre 1 e 50000\n");
            printf("Uso: %s [tamanho_grade] [num_iteracoes]\n", argv[0]);
            printf("Exemplo: %s 512 5000\n", argv[0]);
            return 1;
        }
        ITER = temp_ITER;
    }
    
    printf("=== SIMULAÇÃO DE VISCOSIDADE - NAVIER-STOKES ===\n");
    printf("Grade: %dx%d, Iterações: %d, Viscosidade: %.3f\n", N, N, ITER, NU);
    printf("Argumentos recebidos: N=%d, ITER=%d\n", N, ITER);
    
    // Alocar memória para as matrizes
    initialize_matrices();
    
    pascal_start(1); // Iniciar medição geral da simulação completa
    
    // Inicializar todos os pontos com velocidade zero
    reset_matrices();
    
    // Criar perturbação inicial no centro da grade
    create_perturbation();
    printf("Estado inicial: perturbação criada no centro (tamanho proporcional)\n\n");
    
    // Teste 1: Versão serial (baseline)
    simulate_serial();
    
    // Reinicializar estado para teste paralelo
    reset_matrices();
    create_perturbation();
    
    // Teste 2: Versão paralela static
    simulate_static(4);
    
    // Reinicializar estado para teste collapse
    reset_matrices();
    create_perturbation();
    
    // Teste 3: Versão com collapse
    simulate_collapse(4);
    
    printf("\n=== VERSÕES 4-6: COMPARAÇÃO DE SCHEDULES ===\n");
    
    // Teste diferentes schedules (versões 4, 5 e 6)
    const char* schedules[] = {"static", "dynamic", "guided"};
    const int schedule_versions[] = {4, 5, 6}; // Números das versões
    
    for (int s = 0; s < 3; s++) {
        // Reinicializar estado para cada teste de schedule
        reset_matrices();
        create_perturbation();
        
        printf("=== VERSÃO %d: Testando schedule %s ===\n", schedule_versions[s], schedules[s]);
        omp_set_num_threads(4); // Usar 4 threads fixo
        double start = omp_get_wtime();
        
        pascal_start(schedule_versions[s]); // Iniciar medição para cada schedule
        for (int iter = 0; iter < ITER; iter++) {
            if (s == 0) { // VERSÃO 4: static
                #pragma omp parallel for schedule(static)
                for (int i = 1; i < N-1; i++) {
                    for (int j = 1; j < N-1; j++) {
                        u_new[i][j] = u[i][j] + DT * NU * laplacian(u, i, j);
                        v_new[i][j] = v[i][j] + DT * NU * laplacian(v, i, j);
                    }
                }
            } else if (s == 1) { // VERSÃO 5: dynamic
                #pragma omp parallel for schedule(dynamic)
                for (int i = 1; i < N-1; i++) {
                    for (int j = 1; j < N-1; j++) {
                        u_new[i][j] = u[i][j] + DT * NU * laplacian(u, i, j);
                        v_new[i][j] = v[i][j] + DT * NU * laplacian(v, i, j);
                    }
                }
            } else { // VERSÃO 6: guided
                #pragma omp parallel for schedule(guided)
                for (int i = 1; i < N-1; i++) {
                    for (int j = 1; j < N-1; j++) {
                        u_new[i][j] = u[i][j] + DT * NU * laplacian(u, i, j);
                        v_new[i][j] = v[i][j] + DT * NU * laplacian(v, i, j);
                    }
                }
            }
            
            // Cópia paralela dos valores (mesmo para todos os schedules)
            #pragma omp parallel for
            for (int i = 0; i < N; i++) {
                for (int j = 0; j < N; j++) {
                    u[i][j] = u_new[i][j];
                    v[i][j] = v_new[i][j];
                }
            }
        }
        pascal_stop(schedule_versions[s]); // Finalizar medição para cada schedule
        
        double end = omp_get_wtime();
        printf("Tempo VERSÃO %d (%s): %.4f segundos\n", schedule_versions[s], schedules[s], end - start);
    }
    
    printf("\n=== SIMULAÇÃO CONCLUÍDA ===\n");
    printf("A viscosidade difundiu a perturbação ao longo do tempo.\n");
    
    pascal_stop(1); // Finalizar medição geral da simulação completa
    
    // Liberar memória alocada
    cleanup_matrices();
    
    return 0;
}
