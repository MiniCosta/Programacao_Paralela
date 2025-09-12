# Tarefa 8: Estimativa Estocástica de π com OpenMP

## 📋 Descrição do Problema

Este projeto implementa quatro versões diferentes de estimativa estocástica de π usando o método de Monte Carlo com paralelização OpenMP. O objetivo é comparar diferentes estratégias de sincronização e geradores de números aleatórios.

## 🎯 Objetivos

1. **Implementar estimativa de π** usando método de Monte Carlo
2. **Comparar estratégias de sincronização**:
   - Região crítica (`#pragma omp critical`)
   - Vetorização (cada thread escreve em posição própria)
3. **Comparar geradores de números aleatórios**:
   - `rand()` (não thread-safe)
   - `rand_r()` (thread-safe)

## 🧮 Teoria: Método de Monte Carlo para π

### Fundamento Matemático

O método utiliza a relação entre a área de um círculo e um quadrado:

```
Área do círculo = π × r²
Área do quadrado = (2r)²

Razão = π × r² / (2r)² = π/4
```

### Algoritmo

1. Gerar pontos aleatórios (x,y) no intervalo [0,1]
2. Verificar se o ponto está dentro do círculo: `x² + y² ≤ 1`
3. Contar quantos pontos estão dentro do círculo
4. Estimar π: `π ≈ 4 × (pontos_dentro / total_pontos)`

## 🔧 Implementações

### Versão 1: `rand()` + Região Crítica
```c
#pragma omp parallel num_threads(nthreads)
{
    long long int acertos_priv = 0;
    #pragma omp for
    for (long long int i = 0; i < N; i++) {
        double x = (double)rand() / RAND_MAX;
        double y = (double)rand() / RAND_MAX;
        if (x*x + y*y <= 1.0) acertos_priv++;
    }
    #pragma omp critical
    acertos += acertos_priv;
}
```

**Características:**
- ✅ Cada thread acumula em variável privada
- ✅ Sincronização por região crítica
- ❌ `rand()` não é thread-safe
- ❌ Possível contenção na região crítica

### Versão 2: `rand()` + Vetor Compartilhado
```c
#pragma omp parallel num_threads(nthreads)
{
    int tid = omp_get_thread_num();
    long long int acertos_priv = 0;
    #pragma omp for
    for (long long int i = 0; i < N; i++) {
        double x = (double)rand() / RAND_MAX;
        double y = (double)rand() / RAND_MAX;
        if (x*x + y*y <= 1.0) acertos_priv++;
    }
    acertos_vet[tid] = acertos_priv;
}
// Soma serial após região paralela
for (int i = 0; i < nthreads; i++) acertos_total += acertos_vet[i];
```

**Características:**
- ✅ Elimina contenção (cada thread escreve em posição própria)
- ✅ Soma serial após paralelização
- ❌ `rand()` não é thread-safe
- ✅ Melhor escalabilidade

### Versão 3: `rand_r()` + Região Crítica
```c
#pragma omp parallel num_threads(nthreads)
{
    unsigned int seed = (unsigned int)time(NULL) ^ omp_get_thread_num();
    long long int acertos_priv = 0;
    #pragma omp for
    for (long long int i = 0; i < N; i++) {
        double x = (double)rand_r(&seed) / RAND_MAX;
        double y = (double)rand_r(&seed) / RAND_MAX;
        if (x*x + y*y <= 1.0) acertos_priv++;
    }
    #pragma omp critical
    acertos += acertos_priv;
}
```

**Características:**
- ✅ `rand_r()` é thread-safe
- ✅ Cada thread tem seu próprio seed
- ❌ Possível contenção na região crítica
- ✅ Geração de números aleatórios correta

### Versão 4: `rand_r()` + Vetor Compartilhado
```c
#pragma omp parallel num_threads(nthreads)
{
    int tid = omp_get_thread_num();
    unsigned int seed = (unsigned int)time(NULL) ^ tid;
    long long int acertos_priv = 0;
    #pragma omp for
    for (long long int i = 0; i < N; i++) {
        double x = (double)rand_r(&seed) / RAND_MAX;
        double y = (double)rand_r(&seed) / RAND_MAX;
        if (x*x + y*y <= 1.0) acertos_priv++;
    }
    acertos_vet[tid] = acertos_priv;
}
```

**Características:**
- ✅ `rand_r()` é thread-safe
- ✅ Elimina contenção
- ✅ Melhor escalabilidade
- ✅ Implementação mais robusta

## 📊 Resultados Experimentais

### Configuração do Teste
- **Pontos:** 100.000.000
- **Threads:** 4
- **Compilador:** GCC com `-fopenmp`

### Tempos de Execução

| Versão | Gerador | Sincronização | Tempo (s) | π Estimado | Speedup |
|--------|---------|---------------|-----------|------------|---------|
| 1 | `rand()` | Critical | ~18.0 | 3.1416653 | 1.0x |
| 2 | `rand()` | Vetor | ~17.4 | 3.1413922 | 1.03x |
| 3 | `rand_r()` | Critical | ~0.48 | 3.1416518 | 37.5x |
| 4 | `rand_r()` | Vetor | ~0.44 | 3.1416518 | 40.9x |

### Análise dos Resultados

#### 🚀 Performance Dramática com `rand_r()`
- **37-40x speedup** ao trocar `rand()` por `rand_r()`
- Diferença não é apenas thread-safety, mas contenção interna

#### 🔒 Impacto da Sincronização
- Diferença pequena entre critical e vetor (~8% melhoria)
- Vetor compartilhado tem vantagem em escalabilidade

#### 🎲 Thread-Safety dos Geradores
- `rand()`: Usa estado global compartilhado → contenção severa
- `rand_r()`: Estado local por thread → paralelismo real

## 💡 Conclusões

### Lições Aprendidas

1. **Thread-Safety é Crucial**: A diferença entre `rand()` e `rand_r()` domina completamente a performance
2. **Contenção Interna**: `rand()` tem mutex interno que serializa execução
3. **Estratégia de Sincronização**: Vetor compartilhado é ligeiramente melhor que região crítica
4. **Escalabilidade**: Versão 4 (`rand_r()` + vetor) é a mais escalável

### Recomendações

- **Use sempre `rand_r()`** em código paralelo
- **Prefira vetorização** para reduzir contenção
- **Inicialize seeds únicos** por thread
- **Meça performance** de diferentes estratégias

## 🔨 Como Compilar e Executar

```bash
# Compilar
gcc -fopenmp tarefa8.c -o tarefa8

# Executar com padrões (100M pontos, 4 threads)
./tarefa8

# Executar com parâmetros customizados
./tarefa8 <numero_pontos> <numero_threads>

# Exemplo: 50M pontos, 8 threads
./tarefa8 50000000 8
```

## 📚 Conceitos Importantes

### OpenMP
- **`#pragma omp parallel`**: Cria região paralela
- **`#pragma omp for`**: Distribui iterações entre threads
- **`#pragma omp critical`**: Seção crítica (acesso exclusivo)
- **`omp_get_thread_num()`**: ID da thread atual

### Thread-Safety
- **Thread-safe**: Função pode ser chamada simultaneamente por múltiplas threads
- **Race condition**: Resultado depende da ordem de execução das threads
- **Contenção**: Threads competem por mesmo recurso

### Estratégias de Redução
- **Critical section**: Serializa acesso a variável compartilhada
- **Private accumulation**: Cada thread acumula localmente, depois combina
- **Array indexing**: Cada thread escreve em posição única

## 🎓 Exercícios Propostos

1. Teste com diferentes números de threads (1, 2, 4, 8, 16)
2. Compare com `reduction(+:acertos)` do OpenMP
3. Implemente versão com `drand48_r()`
4. Analise cache misses com ferramentas de profiling
5. Teste em diferentes arquiteturas (ARM, x86)

---
*Este projeto demonstra conceitos fundamentais de programação paralela, thread-safety e otimização de performance em aplicações científicas.*
