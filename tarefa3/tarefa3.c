#include <stdio.h>
#include <stdlib.h>
#include <math.h>     // Para fabs() - cálculo do erro absoluto
#include <time.h>     // Para medição de tempo com clock()

#ifdef _WIN32
    #include <windows.h>
#endif

#define PI_REAL 3.14159265358979323846  // Valor de referência de π com alta precisão

// Série de Leibniz: convergência lenta mas simples π/4 = 1 - 1/3 + 1/5 - 1/7 + ...
double calculate_pi_leibniz(long long iterations) {
    double pi_approx = 0.0;
    int sign = 1;                    // Alterna entre +1 e -1 para os sinais da série
    
    for (long long i = 0; i < iterations; i++) {
        pi_approx += sign * (1.0 / (2 * i + 1));  // Denominadores ímpares: 1, 3, 5, 7...
        sign *= -1;                              // Alterna o sinal a cada iteração
    }
    
    return 4.0 * pi_approx;         // Multiplica por 4 pois calculamos π/4
}

// Série de Nilakantha: convergência mais rápida π = 3 + 4/(2*3*4) - 4/(4*5*6) + 4/(6*7*8) - ...
double calculate_pi_nilakantha(long long iterations) {
    double pi_approx = 3.0;         // Começa com 3, valor base da série
    int sign = 1;                   // Alterna sinais: +, -, +, -, ...
    
    for (long long i = 1; i <= iterations; i++) {
        long long n = 2 * i;        // Gera números pares: 2, 4, 6, 8...
        pi_approx += sign * (4.0 / (n * (n + 1) * (n + 2)));  // Produto de 3 números consecutivos
        sign *= -1;                 // Alterna sinal para próxima iteração
    }
    
    return pi_approx;               // Retorna aproximação direta de π
}

// Medição de tempo usando ponteiros para função - permite testar qualquer algoritmo
double measure_time(double (*func)(long long), long long iterations) {
    clock_t start = clock();        // Marca tempo inicial em ticks do processador
    double result = func(iterations);  // Executa a função passada como parâmetro
    clock_t end = clock();          // Marca tempo final
    
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;  // Converte para segundos
    return time_taken;
}

// Calcula diferença absoluta entre valor real e aproximação
double calculate_error(double approximation) {
    return fabs(PI_REAL - approximation);  // fabs() garante valor positivo
}

// Formatação padronizada dos resultados em tabela organizada
void print_results(const char* method, long long iterations, double pi_approx, double time_taken) {
    double error = calculate_error(pi_approx);                    // Erro absoluto
    double accuracy_percentage = (1.0 - (error / PI_REAL)) * 100.0;  // Precisão percentual
    
    printf("%-15s | %12lld | %15.12f | %10.6f | %12.2e | %8.4f%%\n", 
           method, iterations, pi_approx, time_taken, error, accuracy_percentage);
}

int main() {
    printf("Cálculo de Aproximações de π\n");
    printf("Valor real de π: %.15f\n\n", PI_REAL);
    
    // Teste com diferentes escalas para analisar convergência e performance
    long long test_iterations[] = {100, 1000, 10000, 100000, 1000000, 10000000};
    int num_tests = sizeof(test_iterations) / sizeof(test_iterations[0]);  // Número de testes
    
    printf("%-15s | %12s | %15s | %10s | %12s | %8s\n", 
           "Método", "Iterações", "π Aproximado", "Tempo (s)", "Erro", "Precisão");
    printf("-----------------|--------------|-----------------|-----------|--------------|----------\n");
    
    // Análise da série de Leibniz - convergência lenta mas conceptualmente simples
    printf("\nSérie de Leibniz (π/4 = 1 - 1/3 + 1/5 - 1/7 + ...):\n");
    for (int i = 0; i < num_tests; i++) {
        double pi_approx = calculate_pi_leibniz(test_iterations[i]);       // Calcula aproximação
        double time_taken = measure_time(calculate_pi_leibniz, test_iterations[i]);  // Mede tempo
        print_results("Leibniz", test_iterations[i], pi_approx, time_taken);
    }
    
    // Análise da série de Nilakantha - convergência mais rápida
    printf("\nSérie de Nilakantha (π = 3 + 4/(2×3×4) - 4/(4×5×6) + ...):\n");
    for (int i = 0; i < num_tests; i++) {
        double pi_approx = calculate_pi_nilakantha(test_iterations[i]);    // Calcula aproximação
        double time_taken = measure_time(calculate_pi_nilakantha, test_iterations[i]);  // Mede tempo
        print_results("Nilakantha", test_iterations[i], pi_approx, time_taken);
    }
    
    // Comparação direta entre os dois métodos com mesmo número de iterações
    printf("\n=== ANÁLISE COMPARATIVA ===\n");
    
    long long comparison_iterations = 1000000;  // Número fixo para comparação justa
    printf("\nComparação com %lld iterações:\n", comparison_iterations);
    
    // Testa Leibniz com medições independentes
    double leibniz_pi = calculate_pi_leibniz(comparison_iterations);
    double leibniz_time = measure_time(calculate_pi_leibniz, comparison_iterations);
    double leibniz_error = calculate_error(leibniz_pi);
    
    // Testa Nilakantha com medições independentes
    double nilakantha_pi = calculate_pi_nilakantha(comparison_iterations);
    double nilakantha_time = measure_time(calculate_pi_nilakantha, comparison_iterations);
    double nilakantha_error = calculate_error(nilakantha_pi);
    
    printf("\nLeibniz    : π ≈ %.12f | Erro: %.2e | Tempo: %.6f s\n", 
           leibniz_pi, leibniz_error, leibniz_time);
    printf("Nilakantha : π ≈ %.12f | Erro: %.2e | Tempo: %.6f s\n", 
           nilakantha_pi, nilakantha_error, nilakantha_time);
    return 0;
}