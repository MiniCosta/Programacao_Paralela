#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <string.h>

// Parâmetros da simulação
#define NX 256      // Número de pontos na direção x
#define NY 256      // Número de pontos na direção y
#define DX 1.0      // Espaçamento da grade em x
#define DY 1.0      // Espaçamento da grade em y
#define DT 0.001    // Passo de tempo
#define NU 0.1      // Viscosidade cinemática
#define MAX_ITER 1500  // Número máximo de iterações

// Estrutura para armazenar o campo de velocidade
typedef struct {
    double u[NX][NY];  // Velocidade em x
    double v[NX][NY];  // Velocidade em y
} VelocityField;

// Função para inicializar o campo de velocidade
void initialize_field(VelocityField *field, int mode) {
    #pragma omp parallel for collapse(2)
    for (int i = 0; i < NX; i++) {
        for (int j = 0; j < NY; j++) {
            if (mode == 0) {
                // Campo parado
                field->u[i][j] = 0.0;
                field->v[i][j] = 0.0;
            } else if (mode == 1) {
                // Velocidade constante
                field->u[i][j] = 1.0;
                field->v[i][j] = 0.5;
            } else if (mode == 2) {
                // Campo parado com pequena perturbação no centro
                field->u[i][j] = 0.0;
                field->v[i][j] = 0.0;
                
                // Adicionar perturbação gaussiana no centro
                double center_x = NX / 2.0;
                double center_y = NY / 2.0;
                double dist_sq = (i - center_x) * (i - center_x) + (j - center_y) * (j - center_y);
                double sigma = 5.0;
                
                if (dist_sq < sigma * sigma) {
                    field->u[i][j] = 2.0 * exp(-dist_sq / (2.0 * sigma * sigma));
                    field->v[i][j] = 1.0 * exp(-dist_sq / (2.0 * sigma * sigma));
                }
            }
        }
    }
}

// Função para aplicar condições de contorno (no-slip)
void apply_boundary_conditions(VelocityField *field) {
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            // Bordas horizontais
            for (int i = 0; i < NX; i++) {
                field->u[i][0] = 0.0;
                field->v[i][0] = 0.0;
                field->u[i][NY-1] = 0.0;
                field->v[i][NY-1] = 0.0;
            }
        }
        
        #pragma omp section
        {
            // Bordas verticais
            for (int j = 0; j < NY; j++) {
                field->u[0][j] = 0.0;
                field->v[0][j] = 0.0;
                field->u[NX-1][j] = 0.0;
                field->v[NX-1][j] = 0.0;
            }
        }
    }
}

// Função para calcular a divergência (para verificação)
double calculate_divergence(VelocityField *field) {
    double max_div = 0.0;
    
    #pragma omp parallel for collapse(2) reduction(max:max_div)
    for (int i = 1; i < NX-1; i++) {
        for (int j = 1; j < NY-1; j++) {
            double div = (field->u[i+1][j] - field->u[i-1][j]) / (2.0 * DX) +
                        (field->v[i][j+1] - field->v[i][j-1]) / (2.0 * DY);
            max_div = fmax(max_div, fabs(div));
        }
    }
    
    return max_div;
}

// Função para calcular a energia cinética total
double calculate_kinetic_energy(VelocityField *field) {
    double energy = 0.0;
    
    #pragma omp parallel for collapse(2) reduction(+:energy)
    for (int i = 0; i < NX; i++) {
        for (int j = 0; j < NY; j++) {
            energy += field->u[i][j] * field->u[i][j] + field->v[i][j] * field->v[i][j];
        }
    }
    
    return 0.5 * energy / (NX * NY);
}

// Função para evoluir o campo de velocidade usando diferenças finitas
void evolve_velocity_static(VelocityField *current, VelocityField *next) {
    #pragma omp parallel for collapse(2) schedule(static)
    for (int i = 1; i < NX-1; i++) {
        for (int j = 1; j < NY-1; j++) {
            // Laplaciano de u
            double lap_u = (current->u[i+1][j] - 2.0*current->u[i][j] + current->u[i-1][j]) / (DX*DX) +
                          (current->u[i][j+1] - 2.0*current->u[i][j] + current->u[i][j-1]) / (DY*DY);
            
            // Laplaciano de v
            double lap_v = (current->v[i+1][j] - 2.0*current->v[i][j] + current->v[i-1][j]) / (DX*DX) +
                          (current->v[i][j+1] - 2.0*current->v[i][j] + current->v[i][j-1]) / (DY*DY);
            
            // Equação de Navier-Stokes simplificada (apenas viscosidade)
            next->u[i][j] = current->u[i][j] + DT * NU * lap_u;
            next->v[i][j] = current->v[i][j] + DT * NU * lap_v;
        }
    }
}

void evolve_velocity_dynamic(VelocityField *current, VelocityField *next) {
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int i = 1; i < NX-1; i++) {
        for (int j = 1; j < NY-1; j++) {
            // Laplaciano de u
            double lap_u = (current->u[i+1][j] - 2.0*current->u[i][j] + current->u[i-1][j]) / (DX*DX) +
                          (current->u[i][j+1] - 2.0*current->u[i][j] + current->u[i][j-1]) / (DY*DY);
            
            // Laplaciano de v
            double lap_v = (current->v[i+1][j] - 2.0*current->v[i][j] + current->v[i-1][j]) / (DX*DX) +
                          (current->v[i][j+1] - 2.0*current->v[i][j] + current->v[i][j-1]) / (DY*DY);
            
            // Equação de Navier-Stokes simplificada (apenas viscosidade)
            next->u[i][j] = current->u[i][j] + DT * NU * lap_u;
            next->v[i][j] = current->v[i][j] + DT * NU * lap_v;
        }
    }
}

void evolve_velocity_guided(VelocityField *current, VelocityField *next) {
    #pragma omp parallel for collapse(2) schedule(guided)
    for (int i = 1; i < NX-1; i++) {
        for (int j = 1; j < NY-1; j++) {
            // Laplaciano de u
            double lap_u = (current->u[i+1][j] - 2.0*current->u[i][j] + current->u[i-1][j]) / (DX*DX) +
                          (current->u[i][j+1] - 2.0*current->u[i][j] + current->u[i][j-1]) / (DY*DY);
            
            // Laplaciano de v
            double lap_v = (current->v[i+1][j] - 2.0*current->v[i][j] + current->v[i-1][j]) / (DX*DX) +
                          (current->v[i][j+1] - 2.0*current->v[i][j] + current->v[i][j-1]) / (DY*DY);
            
            // Equação de Navier-Stokes simplificada (apenas viscosidade)
            next->u[i][j] = current->u[i][j] + DT * NU * lap_u;
            next->v[i][j] = current->v[i][j] + DT * NU * lap_v;
        }
    }
}

// Função para salvar o campo de velocidade em arquivo
void save_field_to_file(VelocityField *field, const char *filename, int iteration) {
    char full_filename[256];
    snprintf(full_filename, sizeof(full_filename), "%s_iter_%04d.dat", filename, iteration);
    
    FILE *fp = fopen(full_filename, "w");
    if (fp == NULL) {
        printf("Erro ao abrir arquivo %s\n", full_filename);
        return;
    }
    
    fprintf(fp, "# x y u v magnitude\n");
    for (int i = 0; i < NX; i++) {
        for (int j = 0; j < NY; j++) {
            double x = i * DX;
            double y = j * DY;
            double u = field->u[i][j];
            double v = field->v[i][j];
            double mag = sqrt(u*u + v*v);
            fprintf(fp, "%.6f %.6f %.6f %.6f %.6f\n", x, y, u, v, mag);
        }
        fprintf(fp, "\n");
    }
    
    fclose(fp);
}

// Função principal de simulação
void run_simulation(int mode, const char *schedule_type, int num_threads) {
    VelocityField field1, field2;
    VelocityField *current = &field1;
    VelocityField *next = &field2;
    VelocityField *temp;
    
    printf("\n=== Simulação: Modo %d, Schedule: %s, Threads: %d ===\n", 
           mode, schedule_type, num_threads);
    
    // Configurar número de threads
    omp_set_num_threads(num_threads);
    
    // Inicializar campo
    initialize_field(current, mode);
    
    double start_time = omp_get_wtime();
    
    // Loop principal de simulação
    for (int iter = 0; iter < MAX_ITER; iter++) {
        // Aplicar condições de contorno
        apply_boundary_conditions(current);
        
        // Evoluir campo de velocidade com diferentes schedules
        if (strcmp(schedule_type, "static") == 0) {
            evolve_velocity_static(current, next);
        } else if (strcmp(schedule_type, "dynamic") == 0) {
            evolve_velocity_dynamic(current, next);
        } else if (strcmp(schedule_type, "guided") == 0) {
            evolve_velocity_guided(current, next);
        }
        
        // Aplicar condições de contorno no novo campo
        apply_boundary_conditions(next);
        
        // Trocar ponteiros
        temp = current;
        current = next;
        next = temp;
        
        // Calcular e imprimir estatísticas a cada 250 iterações
        if (iter % 250 == 0) {
            double energy = calculate_kinetic_energy(current);
            double divergence = calculate_divergence(current);
            printf("Iteração %d: Energia = %.6f, Divergência máx = %.6e\n", 
                   iter, energy, divergence);
        }
        
        // Salvar campo a cada 500 iterações
        if (iter % 500 == 0) {
            char filename[128];
            snprintf(filename, sizeof(filename), "field_mode%d_%s", mode, schedule_type);
            save_field_to_file(current, filename, iter);
        }
    }
    
    double end_time = omp_get_wtime();
    double elapsed_time = end_time - start_time;
    
    printf("Tempo de execução: %.4f segundos\n", elapsed_time);
    
    // Estatísticas finais
    double final_energy = calculate_kinetic_energy(current);
    double final_divergence = calculate_divergence(current);
    printf("Energia final: %.6f\n", final_energy);
    printf("Divergência final máxima: %.6e\n", final_divergence);
    
    // Salvar campo final
    char filename[128];
    snprintf(filename, sizeof(filename), "field_mode%d_%s_final", mode, schedule_type);
    save_field_to_file(current, filename, MAX_ITER);
}

int main(int argc, char *argv[]) {
    printf("Simulação de Fluido - Equação de Navier-Stokes Simplificada\n");
    printf("Parâmetros: NX=%d, NY=%d, DT=%.3f, NU=%.3f, MAX_ITER=%d\n", 
           NX, NY, DT, NU, MAX_ITER);
    
    int num_threads = 4;
    if (argc > 1) {
        num_threads = atoi(argv[1]);
    }
    
    printf("Número de threads: %d\n", num_threads);
    printf("Número máximo de threads disponíveis: %d\n", omp_get_max_threads());
    
    // Teste 1: Campo inicialmente parado
    run_simulation(0, "static", num_threads);
    
    // Teste 2: Campo com velocidade constante
    run_simulation(1, "static", num_threads);
    
    // Teste 3: Campo com perturbação
    run_simulation(2, "static", num_threads);
    
    printf("\n=== Comparação de Performance entre Schedules ===\n");
    
    // Comparar diferentes schedules com perturbação
    const char *schedules[] = {"static", "dynamic", "guided"};
    int num_schedules = sizeof(schedules) / sizeof(schedules[0]);
    
    for (int i = 0; i < num_schedules; i++) {
        run_simulation(2, schedules[i], num_threads);
    }
    
    return 0;
}
