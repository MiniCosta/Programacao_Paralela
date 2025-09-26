#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define NUM_REPETICOES 10000        // Número de trocas de mensagens para cada tamanho
#define TAMANHO_MIN 8               // Menor tamanho de mensagem testado
#define TAMANHO_MAX (1024*1024)     // Maior tamanho de mensagem testado (1 MB)

int main(int argc, char *argv[]) {
	int rank, size;
	MPI_Init(&argc, &argv);                    // Inicializa o ambiente MPI
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);     // Obtém o ID do processo atual
	MPI_Comm_size(MPI_COMM_WORLD, &size);     // Obtém o número total de processos

	// Verifica se o programa está sendo executado com exatamente 2 processos
	if (size != 2) {
		if (rank == 0) {
			fprintf(stderr, "Este programa deve ser executado com exatamente 2 processos.\n");
		}
		MPI_Finalize();
		return 1;
	}

	printf("Tamanho (bytes),Tempo total (s),Tempo médio (us)\n");
	// Loop através de diferentes tamanhos de mensagem (potências de 2)
	for (int tam = TAMANHO_MIN; tam <= TAMANHO_MAX; tam *= 2) {
		char *buffer = (char*)malloc(tam);      // Aloca buffer para a mensagem
		memset(buffer, 0, tam);                 // Inicializa buffer com zeros
		double tempo_inicio, tempo_fim, tempo_total = 0.0;

		// Processo 0: responsável por iniciar o ping-pong e medir o tempo
		if (rank == 0) {
			tempo_inicio = MPI_Wtime();             // Inicia cronômetro
			for (int i = 0; i < NUM_REPETICOES; i++) {
				// Envia mensagem sincronamente para processo 1 (ping) - garante que recv iniciou
				MPI_Ssend(buffer, tam, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
				// Recebe resposta do processo 1 (pong)
				MPI_Recv(buffer, tam, MPI_CHAR, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			}
			tempo_fim = MPI_Wtime();                // Para cronômetro
			tempo_total = tempo_fim - tempo_inicio;
			double tempo_medio = (tempo_total / NUM_REPETICOES) * 1e6; // Converte para microssegundos
			printf("%d,%.6f,%.2f\n", tam, tempo_total, tempo_medio);
		// Processo 1: responde às mensagens do processo 0
		} else if (rank == 1) {
			for (int i = 0; i < NUM_REPETICOES; i++) {
				// Recebe mensagem do processo 0 (ping)
				MPI_Recv(buffer, tam, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				// Envia sincronamente a mesma mensagem de volta para processo 0 (pong)
				MPI_Ssend(buffer, tam, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
			}
		}
		free(buffer);                               // Libera memória alocada
	}
	MPI_Finalize();                                 // Finaliza o ambiente MPI
	return 0;
}
