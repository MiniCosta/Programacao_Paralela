#include <stdio.h>  // Para printf, scanf
#include <stdlib.h> // Para funções padrão
#include <math.h>   // Para sqrt()
#include <omp.h>    // Para OpenMP
#include <time.h>   // Para medição de tempo

// Função para verificar se um número é primo
int eh_primo(int n) {
    if (n < 2) return 0;        // Números menores que 2 não são primos
    if (n == 2) return 1;       // 2 é o único primo par
    if (n % 2 == 0) return 0;   // Outros números pares não são primos
    
    int limite = (int)sqrt(n);  // Só precisa testar até raiz quadrada
    for (int i = 3; i <= limite; i += 2) {  // Testa apenas números ímpares
        if (n % i == 0) return 0;            // Se divisível, não é primo
    }
    return 1;  // Se chegou até aqui, é primo
}

// Versão sequencial - executa em uma única thread
int contar_primos_sequencial(int n) {
    int contador = 0;  // Contador seguro (sem concorrência)
    
    for (int i = 2; i <= n; i++) {      // Loop sequencial
        if (eh_primo(i)) {              // Testa se é primo
            contador++;                 // Incrementa sem risco de race condition
        }
    }
    
    return contador;  // Retorna contagem correta
}

// Versão paralela com OpenMP - DEMONSTRA RACE CONDITION
int contar_primos_paralelo(int n) {
    int contador = 0;  // Variável compartilhada entre threads (PERIGOSO!)
    
    #pragma omp parallel for  // Paraleliza o loop SEM reduction
    for (int i = 2; i <= n; i++) {      // Cada thread processa algumas iterações
        if (eh_primo(i)) {              // Teste de primalidade independente
            contador++;                 // RACE CONDITION: acesso não sincronizado!
        }
    }
    
    return contador;  // Retorna contagem INCORRETA devido à race condition
}

// Função para medir tempo de execução usando ponteiro para função
double medir_tempo(int (*funcao)(int), int n) {
    double inicio = omp_get_wtime();  // Marca tempo de início (alta precisão)
    int resultado = funcao(n);        // Executa a função (seq ou par)
    double fim = omp_get_wtime();     // Marca tempo de fim
    return fim - inicio;              // Retorna tempo decorrido em segundos
}

int main() {
    // Fixar número de threads em 4 para testes consistentes
    omp_set_num_threads(4);
    
    printf("\n=== CONTAGEM DE NÚMEROS PRIMOS ===\n");
    printf("Número de threads fixo: %d\n", omp_get_max_threads());  // Confirma configuração
    
    // Array com valores de teste crescentes para demonstrar comportamento
    int valores_n[] = {1000, 10000, 100000, 1000000, 10000000};
    int num_testes = sizeof(valores_n) / sizeof(valores_n[0]);  // Calcula quantidade de testes
    
    printf("\n=== RESULTADOS DOS TESTES ===\n");
    // Cabeçalho da tabela de resultados com formatação alinhada
    printf("%-12s %-12s %-12s %-15s %-15s %-10s %-20s\n", "N", "Primos Seq", "Primos Par", "Tempo Seq (s)", "Tempo Par (s)", "Speedup", "Status");
    printf("%-12s %-12s %-12s %-15s %-15s %-10s %-20s\n", "============", "============", "============", "===============", "===============", "==========", "====================");
    
    // Loop principal de testes com diferentes tamanhos de problema
    for (int i = 0; i < num_testes; i++) {
        int n = valores_n[i];                    // Valor atual de n
        printf("\nTestando com n = %d...\n", n); // Feedback visual do progresso
        
        // Medindo tempo e resultado da versão sequencial (referência correta)
        double tempo_seq = medir_tempo(contar_primos_sequencial, n);
        int primos_seq = contar_primos_sequencial(n);  // Resultado CORRETO
        
        // Medindo tempo e resultado da versão paralela (com race condition)
        double tempo_par = medir_tempo(contar_primos_paralelo, n);
        int primos_par = contar_primos_paralelo(n);    // Resultado INCORRETO
        
        // Calculando speedup aparente (mesmo com resultados incorretos)
        double speedup = (tempo_seq > 0) ? tempo_seq / tempo_par : 0;
        
        // Verificação de correção comparando resultados
        const char* status = (primos_seq == primos_par) ? "CORRETO" : "ERRO - Race Condition";
        
        // Exibindo linha formatada com todos os resultados
        printf("%-12d %-12d %-12d %-15.6f %-15.6f %-10.2f %-20s\n", 
               n, primos_seq, primos_par, tempo_seq, tempo_par, speedup, status);
    } 

    return 0;  // Programa executado com sucesso
}

// Comando de compilação: gcc -fopenmp tarefa5.c -o tarefa5 -lm
