#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>

// Exemplo 1: Limitado por memória (soma de vetores)
void memoria_limitada(int n) {
	double *a = malloc(n * sizeof(double));
	double *b = malloc(n * sizeof(double));
	double *c = malloc(n * sizeof(double));
	for (int i = 0; i < n; i++) {
		a[i] = i * 0.5;
		b[i] = i * 2.0;
	}
	double start = omp_get_wtime();
#pragma omp parallel for
	for (int i = 0; i < n; i++) {
		c[i] = a[i] + b[i];
	}
	double end = omp_get_wtime();
	printf("Tempo (memória): %f segundos\n", end - start);
	printf("Exemplo: c[0]=%.2f, c[n-1]=%.2f\n", c[0], c[n-1]);
	free(a); free(b); free(c);
}

// Exemplo 2: Limitado por CPU (cálculos matemáticos intensivos)
void cpu_limitada(long n) {
	double resultado = 0.0;
	double start = omp_get_wtime();
#pragma omp parallel for reduction(+:resultado)
	for (long i = 1; i <= n; i++) {
		resultado += sin(i) * log(i + 1) / (cos(i) + 2.0);
	}
	double end = omp_get_wtime();
	printf("Tempo (CPU): %f segundos\n", end - start);
	printf("Resultado final: %.5f\n", resultado);
}

int main(int argc, char *argv[]) {

	int n_threads = 2;
	char *env_threads = getenv("OMP_NUM_THREADS");
	if (env_threads != NULL) {
		n_threads = atoi(env_threads);
		printf("OMP_NUM_THREADS detectado: usando %d threads\n", n_threads);
	} else if (argc > 1) {
		n_threads = atoi(argv[1]);
		printf("Argumento de linha de comando: usando %d threads\n", n_threads);
	} else {
		printf("Usando valor padrão: %d threads\n", n_threads);
	}
	omp_set_num_threads(n_threads);

	// Ajuste os tamanhos conforme sua máquina
	int n_mem = 200000000; // 100 milhões
	long n_cpu = 50000000; // 50 milhões

	memoria_limitada(n_mem);
	cpu_limitada(n_cpu);
	return 0;
}

/*
Como compilar:
gcc -fopenmp -O2 -o tarefa4 tarefa4.c -lm

Como executar variando o número de threads:
1. Usando argumento de linha de comando:
	./tarefa4 2   # executa com 2 threads
	./tarefa4 4   # executa com 4 threads
	./tarefa4 8   # executa com 8 threads

2. Usando variável de ambiente OMP_NUM_THREADS (tem prioridade sobre o argumento):
	export OMP_NUM_THREADS=4
	./tarefa4      # executa com 4 threads, ignorando o argumento

Se nenhum argumento for passado e OMP_NUM_THREADS não estiver definida, o padrão é 4 threads.

Altere os valores de n_mem e n_cpu no código para testar diferentes limites de memória/CPU.
*/
