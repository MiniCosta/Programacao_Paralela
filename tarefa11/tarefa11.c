#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

// Parâmetros da simulação
#define N 512        // Grade 512x512 pontos
#define DT 0.00001   // Passo temporal para estabilidade numérica
#define NU 0.1       // Viscosidade cinemática do fluido
#define ITER 5000    // Número total de iterações temporais

double u[N][N], v[N][N];         // Campos de velocidade atuais (u=x, v=y)
double u_new[N][N], v_new[N][N]; // Campos de velocidade para próximo passo

// Calcular laplaciano usando diferenças finitas de 5 pontos
double laplacian(double field[N][N], int i, int j) {
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
    
    double end = omp_get_wtime();
    printf("Tempo VERSÃO 2 (static): %.4f segundos\n", end - start);
}

// VERSÃO 3: Simulação com collapse (combina loops aninhados)
void simulate_collapse(int threads) {
    printf("=== VERSÃO 3: SIMULAÇÃO COLLAPSE (%d threads) ===\n", threads);
    omp_set_num_threads(threads);
    double start = omp_get_wtime();
    
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
    
    double end = omp_get_wtime();
    printf("Tempo VERSÃO 3 (collapse): %.4f segundos\n", end - start);
}

int main(int argc, char *argv[]) {
    // Verificar argumentos da linha de comando
    int num_threads = 4; // Valor padrão
    
    if (argc > 1) {
        num_threads = atoi(argv[1]); // Converter argumento para inteiro
        if (num_threads <= 0 || num_threads > 8) {
            printf("Erro: Número de threads deve estar entre 1 e 8\n");
            printf("Uso: %s [num_threads]\n", argv[0]);
            printf("Exemplo: %s 4\n", argv[0]);
            return 1;
        }
    }
    
    printf("=== SIMULAÇÃO DE VISCOSIDADE - NAVIER-STOKES ===\n");
    printf("Grade: %dx%d, Iterações: %d, Viscosidade: %.3f\n", N, N, ITER, NU);
    printf("Número de threads: %d\n\n", num_threads);
    
    // Inicializar todos os pontos com velocidade zero
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            u[i][j] = v[i][j] = 0.0;
        }
    }
    
    // Criar perturbação quadrada no centro da grade
    int center = N/2; // Ponto central da grade
    for (int i = center-5; i <= center+5; i++) {
        for (int j = center-5; j <= center+5; j++) {
            if (i >= 0 && i < N && j >= 0 && j < N) {
                u[i][j] = 1.0; // Velocidade inicial em x
                v[i][j] = 0.5; // Velocidade inicial em y
            }
        }
    }
    
    printf("Estado inicial: perturbação criada no centro\n\n");
    
    // Teste 1: Versão serial (baseline)
    simulate_serial();
    
    // Reinicializar estado para teste paralelo
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            u[i][j] = v[i][j] = 0.0;
        }
    }
    // Recriar perturbação
    for (int i = center-5; i <= center+5; i++) {
        for (int j = center-5; j <= center+5; j++) {
            if (i >= 0 && i < N && j >= 0 && j < N) {
                u[i][j] = 1.0;
                v[i][j] = 0.5;
            }
        }
    }
    
    // Teste 2: Versão paralela static
    simulate_static(num_threads);
    
    // Reinicializar estado para teste collapse
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            u[i][j] = v[i][j] = 0.0;
        }
    }
    // Recriar perturbação
    for (int i = center-5; i <= center+5; i++) {
        for (int j = center-5; j <= center+5; j++) {
            if (i >= 0 && i < N && j >= 0 && j < N) {
                u[i][j] = 1.0;
                v[i][j] = 0.5;
            }
        }
    }
    
    // Teste 3: Versão com collapse
    simulate_collapse(num_threads);
    
    printf("\n=== VERSÕES 4-6: COMPARAÇÃO DE SCHEDULES ===\n");
    
    // Teste diferentes schedules (versões 4, 5 e 6)
    const char* schedules[] = {"static", "dynamic", "guided"};
    const int schedule_versions[] = {4, 5, 6}; // Números das versões
    
    for (int s = 0; s < 3; s++) {
        // Reinicializar estado para cada teste de schedule
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                u[i][j] = v[i][j] = 0.0;
            }
        }
        // Recriar perturbação idêntica para comparação justa
        for (int i = center-5; i <= center+5; i++) {
            for (int j = center-5; j <= center+5; j++) {
                if (i >= 0 && i < N && j >= 0 && j < N) {
                    u[i][j] = 1.0;
                    v[i][j] = 0.5;
                }
            }
        }
        
        printf("=== VERSÃO %d: Testando schedule %s ===\n", schedule_versions[s], schedules[s]);
        omp_set_num_threads(num_threads); // Usar número de threads especificado
        double start = omp_get_wtime();
        
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
        
        double end = omp_get_wtime();
        printf("Tempo VERSÃO %d (%s): %.4f segundos\n", schedule_versions[s], schedules[s], end - start);
    }
    
    printf("\n=== SIMULAÇÃO CONCLUÍDA ===\n");
    printf("A viscosidade difundiu a perturbação ao longo do tempo.\n");
    
    return 0;
}
