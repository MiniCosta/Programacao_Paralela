#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <mpi.h>
#include <sys/time.h>

#define N_GLOBAL 120000    // Tamanho total da barra (divisivel por 2,3,4,5,6,8,10,12,15,16,20,24)
#define N_TIMESTEPS 10000  // Numero de iteracoes temporais
#define ALPHA 0.1          // Coeficiente de difusao termica
#define DT 0.001           // Passo temporal
#define DX 0.1             // Espacamento espacial

// Função para obter tempo atual
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
}

// Inicializa condições iniciais (pulso de calor no centro)
void inicializar_temperatura(double *temp, int n_local, int offset) {
    for (int i = 0; i < n_local; i++) {
        int pos_global = offset + i;
        if (pos_global >= N_GLOBAL/3 && pos_global <= 2*N_GLOBAL/3) {
            temp[i] = 100.0; // Temperatura alta no centro
        } else {
            temp[i] = 0.0;   // Temperatura baixa nas bordas
        }
    }
}

// Versão 1: Comunicação bloqueante com MPI_Send/MPI_Recv
double simular_bloqueante(int rank, int size) {
    int n_local = N_GLOBAL / size;
    int offset = rank * n_local;
    
    // Arrays com células fantasma (ghost cells)
    double *temp = (double*)calloc(n_local + 2, sizeof(double));
    double *temp_new = (double*)calloc(n_local + 2, sizeof(double));
    
    // Inicializar temperaturas (índices 1 a n_local são dados reais)
    inicializar_temperatura(&temp[1], n_local, offset);
    
    double start_time = get_time();
    
    for (int t = 0; t < N_TIMESTEPS; t++) {
        // Trocar bordas com vizinhos
        if (rank > 0) {
            MPI_Send(&temp[1], 1, MPI_DOUBLE, rank-1, 0, MPI_COMM_WORLD);
            MPI_Recv(&temp[0], 1, MPI_DOUBLE, rank-1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        if (rank < size-1) {
            MPI_Send(&temp[n_local], 1, MPI_DOUBLE, rank+1, 1, MPI_COMM_WORLD);
            MPI_Recv(&temp[n_local+1], 1, MPI_DOUBLE, rank+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        
        // Atualizar temperaturas usando equação de difusão
        for (int i = 1; i <= n_local; i++) {
            temp_new[i] = temp[i] + ALPHA * DT / (DX*DX) * 
                         (temp[i-1] - 2*temp[i] + temp[i+1]);
        }
        
        // Trocar arrays
        double *tmp = temp;
        temp = temp_new;
        temp_new = tmp;
    }
    
    double end_time = get_time();
    
    free(temp);
    free(temp_new);
    
    return end_time - start_time;
}

// Versão 2: Comunicação não-bloqueante com MPI_Isend/MPI_Irecv + MPI_Wait
double simular_nao_bloqueante_wait(int rank, int size) {
    int n_local = N_GLOBAL / size;
    int offset = rank * n_local;
    
    double *temp = (double*)calloc(n_local + 2, sizeof(double));
    double *temp_new = (double*)calloc(n_local + 2, sizeof(double));
    
    inicializar_temperatura(&temp[1], n_local, offset);
    
    double start_time = get_time();
    
    for (int t = 0; t < N_TIMESTEPS; t++) {
        MPI_Request req[4];
        int req_count = 0;
        
        // Iniciar comunicações não-bloqueantes
        if (rank > 0) {
            MPI_Isend(&temp[1], 1, MPI_DOUBLE, rank-1, 0, MPI_COMM_WORLD, &req[req_count++]);
            MPI_Irecv(&temp[0], 1, MPI_DOUBLE, rank-1, 1, MPI_COMM_WORLD, &req[req_count++]);
        }
        if (rank < size-1) {
            MPI_Isend(&temp[n_local], 1, MPI_DOUBLE, rank+1, 1, MPI_COMM_WORLD, &req[req_count++]);
            MPI_Irecv(&temp[n_local+1], 1, MPI_DOUBLE, rank+1, 0, MPI_COMM_WORLD, &req[req_count++]);
        }
        
        // Aguardar todas as comunicações
        MPI_Waitall(req_count, req, MPI_STATUSES_IGNORE);
        
        // Atualizar temperaturas
        for (int i = 1; i <= n_local; i++) {
            temp_new[i] = temp[i] + ALPHA * DT / (DX*DX) * 
                         (temp[i-1] - 2*temp[i] + temp[i+1]);
        }
        
        double *tmp = temp;
        temp = temp_new;
        temp_new = tmp;
    }
    
    double end_time = get_time();
    
    free(temp);
    free(temp_new);
    
    return end_time - start_time;
}

// Versão 3: Comunicação não-bloqueante com MPI_Test (sobreposição computação/comunicação)
double simular_nao_bloqueante_test(int rank, int size) {
    int n_local = N_GLOBAL / size;
    int offset = rank * n_local;
    
    double *temp = (double*)calloc(n_local + 2, sizeof(double));
    double *temp_new = (double*)calloc(n_local + 2, sizeof(double));
    
    inicializar_temperatura(&temp[1], n_local, offset);
    
    double start_time = get_time();
    
    for (int t = 0; t < N_TIMESTEPS; t++) {
        MPI_Request req[4];
        int req_count = 0;
        
        // Iniciar comunicações não-bloqueantes
        if (rank > 0) {
            MPI_Isend(&temp[1], 1, MPI_DOUBLE, rank-1, 0, MPI_COMM_WORLD, &req[req_count++]);
            MPI_Irecv(&temp[0], 1, MPI_DOUBLE, rank-1, 1, MPI_COMM_WORLD, &req[req_count++]);
        }
        if (rank < size-1) {
            MPI_Isend(&temp[n_local], 1, MPI_DOUBLE, rank+1, 1, MPI_COMM_WORLD, &req[req_count++]);
            MPI_Irecv(&temp[n_local+1], 1, MPI_DOUBLE, rank+1, 0, MPI_COMM_WORLD, &req[req_count++]);
        }
        
        // Atualizar pontos internos primeiro (não precisam das bordas)
        for (int i = 2; i <= n_local-1; i++) {
            temp_new[i] = temp[i] + ALPHA * DT / (DX*DX) * 
                         (temp[i-1] - 2*temp[i] + temp[i+1]);
        }
        
        // Aguardar comunicações usando MPI_Test em loop
        int all_complete = 0;
        while (!all_complete) {
            int flag;
            MPI_Testall(req_count, req, &flag, MPI_STATUSES_IGNORE);
            all_complete = flag;
            
            // Pode fazer outras computações aqui se necessário
            if (!all_complete) {
                // Simula trabalho adicional durante espera
                volatile double dummy = 0.0;
                for (int k = 0; k < 100; k++) dummy += k * 0.001;
            }
        }
        
        // Atualizar pontos das bordas (precisam dos valores dos vizinhos)
        if (n_local > 1) {
            temp_new[1] = temp[1] + ALPHA * DT / (DX*DX) * 
                         (temp[0] - 2*temp[1] + temp[2]);
            temp_new[n_local] = temp[n_local] + ALPHA * DT / (DX*DX) * 
                               (temp[n_local-1] - 2*temp[n_local] + temp[n_local+1]);
        } else {
            // Caso especial: apenas um ponto por processo
            temp_new[1] = temp[1] + ALPHA * DT / (DX*DX) * 
                         (temp[0] - 2*temp[1] + temp[2]);
        }
        
        double *tmp = temp;
        temp = temp_new;
        temp_new = tmp;
    }
    
    double end_time = get_time();
    
    free(temp);
    free(temp_new);
    
    return end_time - start_time;
}

int main(int argc, char *argv[]) {
    int rank, size;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (N_GLOBAL % size != 0) {
        if (rank == 0) {
            printf("Erro: N_GLOBAL (%d) deve ser divisivel pelo numero de processos (%d)\n", 
                   N_GLOBAL, size);
        }
        MPI_Finalize();
        return 1;
    }
    
    if (rank == 0) {
        printf("\n");
        printf("====================================================\n");
        printf("     SIMULACAO DE DIFUSAO DE CALOR 1D - MPI\n");
        printf("====================================================\n");
        printf("Tamanho da barra:      %d pontos\n", N_GLOBAL);
        printf("Numero de processos:   %d\n", size);
        printf("Pontos por processo:   %d\n", N_GLOBAL/size);
        printf("Numero de iteracoes:   %d\n", N_TIMESTEPS);
        printf("Coef. difusao termica: %.3f\n", ALPHA);
        printf("Passo temporal (dt):   %.6f\n", DT);
        printf("Espacamento (dx):      %.3f\n", DX);
        printf("====================================================\n\n");
    }
    
    // Executar as três versões
    double tempo1, tempo2, tempo3;
    
    // Sincronizar antes de cada teste
    MPI_Barrier(MPI_COMM_WORLD);
    tempo1 = simular_bloqueante(rank, size);
    
    MPI_Barrier(MPI_COMM_WORLD);
    tempo2 = simular_nao_bloqueante_wait(rank, size);
    
    MPI_Barrier(MPI_COMM_WORLD);
    tempo3 = simular_nao_bloqueante_test(rank, size);
    
    // Reduzir tempos para o processo 0
    double tempo1_max, tempo2_max, tempo3_max;
    MPI_Reduce(&tempo1, &tempo1_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&tempo2, &tempo2_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&tempo3, &tempo3_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        printf("RESULTADOS DE PERFORMANCE:\n");
        printf("--------------------------------------------------\n");
        printf("%-45s %10.6f s\n", "1. MPI_Send/MPI_Recv (bloqueante):", tempo1_max);
        printf("%-45s %10.6f s\n", "2. MPI_Isend/MPI_Irecv + MPI_Wait:", tempo2_max);
        printf("%-45s %10.6f s\n", "3. MPI_Isend/MPI_Irecv + MPI_Test:", tempo3_max);
        printf("--------------------------------------------------\n");
        
        // Calcular GFLOPS (operacoes de ponto flutuante por segundo)
        double ops_per_timestep = (double)N_GLOBAL * 5.0; // 5 operacoes por ponto por iteracao
        double total_ops = ops_per_timestep * N_TIMESTEPS;
        double gflops1 = (total_ops / tempo1_max) / 1e9;
        double gflops2 = (total_ops / tempo2_max) / 1e9;
        double gflops3 = (total_ops / tempo3_max) / 1e9;
        
        printf("\nPERFORMANCE (GFLOPS):\n");
        printf("--------------------------------------------------\n");
        printf("%-45s %10.2f GFLOPS\n", "1. Comunicacao bloqueante:", gflops1);
        printf("%-45s %10.2f GFLOPS\n", "2. Nao-bloqueante + Wait:", gflops2);
        printf("%-45s %10.2f GFLOPS\n", "3. Nao-bloqueante + Test:", gflops3);
        printf("--------------------------------------------------\n");
        
        printf("\nSPEEDUP RELATIVO:\n");
        printf("--------------------------------------------------\n");
        printf("%-30s %15.2fx\n", "Metodo 2 vs 1:", tempo1_max / tempo2_max);
        printf("%-30s %15.2fx\n", "Metodo 3 vs 1:", tempo1_max / tempo3_max);
        printf("%-30s %15.2fx\n", "Metodo 3 vs 2:", tempo2_max / tempo3_max);
        printf("--------------------------------------------------\n");
        
        printf("\nANALISE DE EFICIENCIA:\n");
        printf("--------------------------------------------------\n");
        double min_tempo = (tempo1_max < tempo2_max) ? tempo1_max : tempo2_max;
        min_tempo = (min_tempo < tempo3_max) ? min_tempo : tempo3_max;
        
        if (min_tempo == tempo1_max) {
            printf("* MELHOR: Comunicacao bloqueante (%.6f s)\n", tempo1_max);
            printf("  - Menor overhead de sincronizacao\n");
            printf("  - Ideal para poucos processos\n");
        } else if (min_tempo == tempo2_max) {
            printf("* MELHOR: Comunicacao nao-bloqueante + Wait (%.6f s)\n", tempo2_max);
            printf("  - Boa sobreposicao computacao/comunicacao\n");
            printf("  - Ideal para muitos processos\n");
        } else {
            printf("* MELHOR: Comunicacao nao-bloqueante + Test (%.6f s)\n", tempo3_max);
            printf("  - Maxima flexibilidade de escalonamento\n");
            printf("  - Ideal para sistemas heterogeneos\n");
        }
        printf("--------------------------------------------------\n");
        
        // Estatisticas adicionais
        double efficiency = (double)size / (tempo1_max * size / tempo1_max); // Aproximacao
        printf("\nESTATISTICAS ADICIONAIS:\n");
        printf("--------------------------------------------------\n");
        printf("Total de operacoes:           %.2e\n", total_ops);
        printf("Operacoes por processo:       %.2e\n", total_ops / size);
        printf("Dados por processo:           %.1f KB\n", ((double)(N_GLOBAL/size + 2) * sizeof(double)) / 1024.0);
        printf("Comunicacoes por timestep:    %d\n", (size > 1) ? 2*(size-1) : 0);
        printf("Total de comunicacoes:        %d\n", ((size > 1) ? 2*(size-1) : 0) * N_TIMESTEPS);
        printf("====================================================\n");
    }
    
    MPI_Finalize();
    return 0;
}
