#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>

// Exemplo 1: Limitado por memória (soma de vetores)
void memoria_limitada(int n) {
	// Aloca três vetores grandes na memória
	double *a = malloc(n * sizeof(double));
	double *b = malloc(n * sizeof(double));
	double *c = malloc(n * sizeof(double));
	
	// Inicializa os vetores com dados
	for (int i = 0; i < n; i++) {
		a[i] = i * 0.5;
		b[i] = i * 2.0;
	}
	
	double start = omp_get_wtime(); // Marca o tempo inicial
#pragma omp parallel for
	for (int i = 0; i < n; i++) {
		// Soma simples, limitada pela largura de banda da memória
		c[i] = a[i] + b[i];
	}
	double end = omp_get_wtime(); // Marca o tempo final
	printf("Memory-bound: %.3f s\n", end - start);
	
	// Libera a memória alocada
	free(a); free(b); free(c);
}

// Exemplo 2: Limitado por CPU (cálculos matemáticos intensivos)
void cpu_limitada(long n) {
	double start = omp_get_wtime(); // Marca o tempo inicial
#pragma omp parallel for
	for (long i = 1; i <= n; i++) {
		// Operações matemáticas intensivas para cada elemento
		double temp = sin(i) * log(i + 1) / (cos(i) + 2.0);
		// Não fazemos soma global para evitar clausulas avançadas
	}
	double end = omp_get_wtime(); // Marca o tempo final
	printf("Compute-bound: %.3f s\n", end - start);
}

int main(int argc, char *argv[]) {
	// Configuração do número de threads por prioridade
	int n_threads = 2; // Valor padrão
	char *env_threads = getenv("OMP_NUM_THREADS");
	
	// Prioridade 1: Variável de ambiente
	if (env_threads != NULL) {
		n_threads = atoi(env_threads);
	// Prioridade 2: Argumento da linha de comando
	} else if (argc > 1) {
		n_threads = atoi(argv[1]);
	}
	
	printf("\n=== OpenMP Performance Test (%d threads) ===\n", n_threads);
	
	// Define o número de threads para OpenMP
	omp_set_num_threads(n_threads);

	// Tamanhos dos problemas - ajuste conforme sua máquina
	int n_mem = 100000000; // 100 milhões de elementos (~2.4GB total memory-bound)
	long n_cpu = 20000000; // 20 milhões de operações (compute-bound)

	// Executa os dois testes
	memoria_limitada(n_mem);
	cpu_limitada(n_cpu);
	printf("=========================================\n\n");
	return 0;
}

