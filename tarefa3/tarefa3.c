#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
#endif

#define PI_REAL 3.14159265358979323846

// Função para calcular π usando a série de Leibniz: π/4 = 1 - 1/3 + 1/5 - 1/7 + ...
double calculate_pi_leibniz(long long iterations) {
    double pi_approx = 0.0;
    int sign = 1;
    
    for (long long i = 0; i < iterations; i++) {
        pi_approx += sign * (1.0 / (2 * i + 1));
        sign *= -1;
    }
    
    return 4.0 * pi_approx;
}

// Função para calcular π usando a série de Nilakantha: π = 3 + 4/(2*3*4) - 4/(4*5*6) + 4/(6*7*8) - ...
double calculate_pi_nilakantha(long long iterations) {
    double pi_approx = 3.0;
    int sign = 1;
    
    for (long long i = 1; i <= iterations; i++) {
        long long n = 2 * i;
        pi_approx += sign * (4.0 / (n * (n + 1) * (n + 2)));
        sign *= -1;
    }
    
    return pi_approx;
}

// Função para medir tempo de execução
double measure_time(double (*func)(long long), long long iterations) {
    clock_t start = clock();
    double result = func(iterations);
    clock_t end = clock();
    
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    return time_taken;
}

// Função para calcular o erro absoluto
double calculate_error(double approximation) {
    return fabs(PI_REAL - approximation);
}

// Função para imprimir resultados formatados
void print_results(const char* method, long long iterations, double pi_approx, double time_taken) {
    double error = calculate_error(pi_approx);
    double accuracy_percentage = (1.0 - (error / PI_REAL)) * 100.0;
    
    printf("%-15s | %12lld | %15.12f | %10.6f | %12.2e | %8.4f%%\n", 
           method, iterations, pi_approx, time_taken, error, accuracy_percentage);
}

int main() {
    printf("Cálculo de Aproximações de π\n");
    printf("Valor real de π: %.15f\n\n", PI_REAL);
    
    // Array com diferentes números de iterações para teste
    long long test_iterations[] = {100, 1000, 10000, 100000, 1000000, 10000000};
    int num_tests = sizeof(test_iterations) / sizeof(test_iterations[0]);
    
    printf("%-15s | %12s | %15s | %10s | %12s | %8s\n", 
           "Método", "Iterações", "π Aproximado", "Tempo (s)", "Erro", "Precisão");
    printf("-----------------|--------------|-----------------|-----------|--------------|----------\n");
    
    // Teste com série de Leibniz
    printf("\nSérie de Leibniz (π/4 = 1 - 1/3 + 1/5 - 1/7 + ...):\n");
    for (int i = 0; i < num_tests; i++) {
        double pi_approx = calculate_pi_leibniz(test_iterations[i]);
        double time_taken = measure_time(calculate_pi_leibniz, test_iterations[i]);
        print_results("Leibniz", test_iterations[i], pi_approx, time_taken);
    }
    
    printf("\nSérie de Nilakantha (π = 3 + 4/(2×3×4) - 4/(4×5×6) + ...):\n");
    for (int i = 0; i < num_tests; i++) {
        double pi_approx = calculate_pi_nilakantha(test_iterations[i]);
        double time_taken = measure_time(calculate_pi_nilakantha, test_iterations[i]);
        print_results("Nilakantha", test_iterations[i], pi_approx, time_taken);
    }
    
    // Análise comparativa
    printf("\n=== ANÁLISE COMPARATIVA ===\n");
    
    long long comparison_iterations = 1000000;
    printf("\nComparação com %lld iterações:\n", comparison_iterations);
    
    double leibniz_pi = calculate_pi_leibniz(comparison_iterations);
    double leibniz_time = measure_time(calculate_pi_leibniz, comparison_iterations);
    double leibniz_error = calculate_error(leibniz_pi);
    
    double nilakantha_pi = calculate_pi_nilakantha(comparison_iterations);
    double nilakantha_time = measure_time(calculate_pi_nilakantha, comparison_iterations);
    double nilakantha_error = calculate_error(nilakantha_pi);
    
    printf("\nLeibniz    : π ≈ %.12f | Erro: %.2e | Tempo: %.6f s\n", 
           leibniz_pi, leibniz_error, leibniz_time);
    printf("Nilakantha : π ≈ %.12f | Erro: %.2e | Tempo: %.6f s\n", 
           nilakantha_pi, nilakantha_error, nilakantha_time);
    
    printf("\nConclusões:\n");
    printf("- A série de Nilakantha converge mais rapidamente que a de Leibniz\n");
    printf("- Mais iterações sempre resultam em maior precisão\n");
    printf("- O tempo de execução cresce linearmente com o número de iterações\n");
    printf("- Para a mesma quantidade de iterações, Nilakantha é mais precisa\n");
    
    return 0;
}

/*
COMPILAÇÃO E EXECUÇÃO:

Linux/macOS:
    chcp 65001  # Sets UTF-8 encoding
    gcc -o tarefa3 tarefa3.c -lm
    ./tarefa3
Windows: 
    chcp 65001  # Sets UTF-8 encoding
    gcc -o tarefa3.exe tarefa3.c -lm
    tarefa3.exe

- No Linux/macOS, a flag -lm é necessária para linkar a biblioteca matemática
*/
