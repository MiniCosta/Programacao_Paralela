#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

// Parâmetros da simulação
#define N 512        // Grade 512x512
#define DT 0.00001   // Passo temporal maior
#define NU 0.1       // Viscosidade
#define ITER 5000   // Número de iterações

double u[N][N], v[N][N];         // Campos atuais
double u_new[N][N], v_new[N][N]; // Campos novos

// Calcular laplaciano usando diferenças finitas
double laplacian(double field[N][N], int i, int j) {
    if (i == 0 || i == N-1 || j == 0 || j == N-1) return 0.0;
    return field[i-1][j] + field[i+1][j] + field[i][j-1] + field[i][j+1] - 4*field[i][j];
}

// Simulação serial
void simulate_serial() {
    printf("=== SIMULAÇÃO SERIAL ===\n");
    double start = omp_get_wtime();
    
    for (int iter = 0; iter < ITER; iter++) {
        for (int i = 1; i < N-1; i++) {
            for (int j = 1; j < N-1; j++) {
                // Equação de difusão: du/dt = ν * ∇²u
                u_new[i][j] = u[i][j] + DT * NU * laplacian(u, i, j);
                v_new[i][j] = v[i][j] + DT * NU * laplacian(v, i, j);
            }
        }
        
        // Copiar valores
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                u[i][j] = u_new[i][j];
                v[i][j] = v_new[i][j];
            }
        }
    }
    
    double end = omp_get_wtime();
    printf("Tempo serial: %.4f segundos\n", end - start);
}

// Simulação paralela com static schedule
void simulate_static(int threads) {
    printf("=== SIMULAÇÃO STATIC (%d threads) ===\n", threads);
    omp_set_num_threads(threads);
    double start = omp_get_wtime();
    
    for (int iter = 0; iter < ITER; iter++) {
        #pragma omp parallel for schedule(static)
        for (int i = 1; i < N-1; i++) {
            for (int j = 1; j < N-1; j++) {
                u_new[i][j] = u[i][j] + DT * NU * laplacian(u, i, j);
                v_new[i][j] = v[i][j] + DT * NU * laplacian(v, i, j);
            }
        }
        
        #pragma omp parallel for
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                u[i][j] = u_new[i][j];
                v[i][j] = v_new[i][j];
            }
        }
    }
    
    double end = omp_get_wtime();
    printf("Tempo static: %.4f segundos\n", end - start);
}

// Simulação com collapse
void simulate_collapse(int threads) {
    printf("=== SIMULAÇÃO COLLAPSE (%d threads) ===\n", threads);
    omp_set_num_threads(threads);
    double start = omp_get_wtime();
    
    for (int iter = 0; iter < ITER; iter++) {
        #pragma omp parallel for schedule(static) collapse(2)
        for (int i = 1; i < N-1; i++) {
            for (int j = 1; j < N-1; j++) {
                u_new[i][j] = u[i][j] + DT * NU * laplacian(u, i, j);
                v_new[i][j] = v[i][j] + DT * NU * laplacian(v, i, j);
            }
        }
        
        #pragma omp parallel for collapse(2)
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                u[i][j] = u_new[i][j];
                v[i][j] = v_new[i][j];
            }
        }
    }
    
    double end = omp_get_wtime();
    printf("Tempo collapse: %.4f segundos\n", end - start);
}

int main() {
    printf("=== SIMULAÇÃO DE VISCOSIDADE - NAVIER-STOKES ===\n");
    printf("Grade: %dx%d, Iterações: %d, Viscosidade: %.3f\n\n", N, N, ITER, NU);
    
    // Inicializar com perturbação no centro
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            u[i][j] = v[i][j] = 0.0;
        }
    }
    
    // Criar perturbação
    int center = N/2;
    for (int i = center-5; i <= center+5; i++) {
        for (int j = center-5; j <= center+5; j++) {
            if (i >= 0 && i < N && j >= 0 && j < N) {
                u[i][j] = 1.0;
                v[i][j] = 0.5;
            }
        }
    }
    
    printf("Estado inicial: perturbação criada no centro\n\n");
    
    // Teste 1: Serial
    simulate_serial();
    
    // Reinicializar
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            u[i][j] = v[i][j] = 0.0;
        }
    }
    for (int i = center-5; i <= center+5; i++) {
        for (int j = center-5; j <= center+5; j++) {
            if (i >= 0 && i < N && j >= 0 && j < N) {
                u[i][j] = 1.0;
                v[i][j] = 0.5;
            }
        }
    }
    
    // Teste 2: Paralelo static
    simulate_static(4);
    
    // Reinicializar
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            u[i][j] = v[i][j] = 0.0;
        }
    }
    for (int i = center-5; i <= center+5; i++) {
        for (int j = center-5; j <= center+5; j++) {
            if (i >= 0 && i < N && j >= 0 && j < N) {
                u[i][j] = 1.0;
                v[i][j] = 0.5;
            }
        }
    }
    
    // Teste 3: Paralelo com collapse
    simulate_collapse(4);
    
    printf("\n=== COMPARAÇÃO DE SCHEDULES ===\n");
    
    // Teste diferentes schedules
    const char* schedules[] = {"static", "dynamic", "guided"};
    
    for (int s = 0; s < 3; s++) {
        // Reinicializar
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                u[i][j] = v[i][j] = 0.0;
            }
        }
        for (int i = center-5; i <= center+5; i++) {
            for (int j = center-5; j <= center+5; j++) {
                if (i >= 0 && i < N && j >= 0 && j < N) {
                    u[i][j] = 1.0;
                    v[i][j] = 0.5;
                }
            }
        }
        
        printf("Testando schedule %s...\n", schedules[s]);
        omp_set_num_threads(4);
        double start = omp_get_wtime();
        
        for (int iter = 0; iter < ITER; iter++) {
            if (s == 0) { // static
                #pragma omp parallel for schedule(static)
                for (int i = 1; i < N-1; i++) {
                    for (int j = 1; j < N-1; j++) {
                        u_new[i][j] = u[i][j] + DT * NU * laplacian(u, i, j);
                        v_new[i][j] = v[i][j] + DT * NU * laplacian(v, i, j);
                    }
                }
            } else if (s == 1) { // dynamic
                #pragma omp parallel for schedule(dynamic)
                for (int i = 1; i < N-1; i++) {
                    for (int j = 1; j < N-1; j++) {
                        u_new[i][j] = u[i][j] + DT * NU * laplacian(u, i, j);
                        v_new[i][j] = v[i][j] + DT * NU * laplacian(v, i, j);
                    }
                }
            } else { // guided
                #pragma omp parallel for schedule(guided)
                for (int i = 1; i < N-1; i++) {
                    for (int j = 1; j < N-1; j++) {
                        u_new[i][j] = u[i][j] + DT * NU * laplacian(u, i, j);
                        v_new[i][j] = v[i][j] + DT * NU * laplacian(v, i, j);
                    }
                }
            }
            
            #pragma omp parallel for
            for (int i = 0; i < N; i++) {
                for (int j = 0; j < N; j++) {
                    u[i][j] = u_new[i][j];
                    v[i][j] = v_new[i][j];
                }
            }
        }
        
        double end = omp_get_wtime();
        printf("Tempo %s: %.4f segundos\n", schedules[s], end - start);
    }
    
    printf("\n=== SIMULAÇÃO CONCLUÍDA ===\n");
    printf("A viscosidade difundiu a perturbação ao longo do tempo.\n");
    
    return 0;
}
