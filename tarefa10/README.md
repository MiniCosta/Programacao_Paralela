# Tarefa 10: Comparação de Mecanismos de Sincronização em OpenMP

## Descrição

Este projeto implementa e compara cinco diferentes abordagens para paralelização do algoritmo de Monte Carlo para estimativa de π, explorando diversos mecanismos de sincronização em OpenMP. O objetivo é analisar o desempenho, produtividade e aplicabilidade de cada técnica.

## Implementações

### 1. Contador Compartilhado com `#pragma omp critical`
```c
#pragma omp critical
acertos_critical++;
```
- **Sincronização**: Região crítica bloqueia acesso concorrente
- **Overhead**: Alto - serialização total de incrementos
- **Uso**: Proteção de código complexo que não pode ser atomizado

### 2. Contador Compartilhado com `#pragma omp atomic`
```c
#pragma omp atomic
acertos_atomic++;
```
- **Sincronização**: Operação atômica em hardware
- **Overhead**: Médio - contenda por variável compartilhada
- **Uso**: Operações simples (incremento, soma, etc.)

### 3. Contadores Privados (Redução Manual com Atomic)
```c
long long int local = 0;
// ... loop com contador local
#pragma omp atomic
acertos_privado += local;
```
- **Sincronização**: Mínima - apenas na redução final
- **Overhead**: Baixo - uma sincronização por thread
- **Uso**: Quando reduction não está disponível

### 4. Vetor de Contadores Privados
```c
acertos_vet[tid] = local;
// ... redução sequencial posterior
```
- **Sincronização**: Nenhuma durante cálculo
- **Overhead**: Mínimo - sem contenção
- **Uso**: Controle total sobre redução e debugging

### 5. Cláusula `reduction`
```c
#pragma omp parallel for reduction(+:acertos_reduction)
```
- **Sincronização**: Automática pelo compilador
- **Overhead**: Otimizado - implementação eficiente
- **Uso**: Padrões de redução conhecidos

## Resultados Experimentais

**Configuração**: 100M pontos, 4 threads, gcc padrão (O1)

| Versão | Mecanismo | π Estimado | Tempo (s) | Speedup | Eficiência |
|--------|-----------|------------|-----------|---------|------------|
| 1 | Critical | 3.1414752800 | 7.908 | 1.0x | 25% |
| 2 | Atomic | 3.1417220800 | 2.745 | 2.9x | 72% |
| 3 | Privado | 3.1417220800 | 0.246 | 32.1x | **803%** |
| 4 | Vetor | 3.1417220800 | 0.225 | **35.1x** | **878%** |
| 5 | Reduction | 3.1415858400 | 0.231 | 34.2x | 855% |

### Análise dos Resultados

#### Desempenho
- **Critical**: Performance ruim devido à serialização total
- **Atomic**: Melhoria significativa, mas ainda com contenção
- **Privados/Reduction**: Performance ótimas com paralelização real

#### Precisão
- Todas as versões produzem estimativas razoáveis de π ≈ 3.14159
- Variação devido à natureza estocástica do Monte Carlo

## Teoria dos Mecanismos de Sincronização

### 1. Regiões Críticas (`#pragma omp critical`)

**Características Técnicas:**
- Exclusão mútua garantida para seção de código arbitrária
- Implementação baseada em mutex/locks internos do OpenMP
- Pode proteger múltiplas instruções e chamadas de função
- Suporte para regiões críticas nomeadas para granularidade específica

**Implementação Interna:**
```c
// O que o compilador gera (conceptualmente)
static omp_lock_t __critical_lock__;
// ...
omp_set_lock(&__critical_lock__);
// código da região crítica
omp_unset_lock(&__critical_lock__);
```

**Sintaxe Avançada:**
```c
// Região crítica global (todas threads competem)
#pragma omp critical
{
    printf("Thread %d executando\n", omp_get_thread_num());
}

// Região crítica nomeada (independente de outras)
#pragma omp critical(arquivo1)
{
    fprintf(file1, "dados thread %d\n", omp_get_thread_num());
}

#pragma omp critical(arquivo2)  // Pode executar simultaneamente com arquivo1
{
    fprintf(file2, "outros dados\n");
}
```

**Casos de Uso Específicos:**
- **I/O compartilhado**: Escrita em arquivos, printf
- **Estruturas de dados complexas**: Listas ligadas, árvores
- **Chamadas de biblioteca não thread-safe**: malloc, algumas APIs
- **Debugging**: Logs e traces consistentes

**Vantagens:**
- Proteção robusta de código complexo multi-instrução
- Facilidade de uso - apenas envolver código existente
- Debugging mais simples - seção executa atomicamente
- Suporte a qualquer tipo de operação

**Desvantagens:**
- Alto overhead de sincronização (lock/unlock)
- Serialização total - apenas uma thread por vez
- Não escalável com muitas threads
- Pode causar deadlocks se mal usado

**Overhead Estimado:** ~100-1000 ciclos de CPU por entrada/saída

### 2. Operações Atômicas (`#pragma omp atomic`)

**Características Técnicas:**
- Operações indivisíveis implementadas em hardware (instruções CPU)
- Garantia de atomicidade sem locks explícitos
- Suporte limitado a operações específicas suportadas pelo hardware
- Implementação otimizada usando instruções como LOCK, CMPXCHG (x86)

**Tipos de Operações Suportadas:**
```c
// Operações básicas (OpenMP 2.0+)
#pragma omp atomic
x++;                    // Incremento
#pragma omp atomic
x--;                    // Decremento
#pragma omp atomic
x += expr;              // Soma
#pragma omp atomic
x *= expr;              // Multiplicação

// Operações estendidas (OpenMP 3.1+)
#pragma omp atomic read
v = x;                  // Leitura atômica

#pragma omp atomic write  
x = expr;               // Escrita atômica

#pragma omp atomic update
x = x + expr;           // Atualização

#pragma omp atomic capture
{
    v = x;              // Captura valor antigo
    x += expr;          // E atualiza
}
```

**Implementação em Assembly (x86-64):**
```assembly
; #pragma omp atomic
; counter++;
lock incq counter(%rip)    ; Instrução atômica direta

; #pragma omp atomic
; counter += value;
mov %rax, value            ; Carrega valor
lock addq %rax, counter(%rip)  ; Soma atômica
```

**Memory Ordering:**
```c
// Controle de ordenação de memória (OpenMP 5.0+)
#pragma omp atomic seq_cst    // Sequential consistency (padrão)
x++;

#pragma omp atomic relaxed   // Relaxed ordering (mais rápido)
y++;

#pragma omp atomic acq_rel   // Acquire-release
z++;
```

**Vantagens:**
- Overhead muito menor que critical (~10-50 ciclos)
- Implementação eficiente em hardware
- Não requer locks explícitos
- Boa escalabilidade para operações simples

**Desvantagens:**
- Limitado a operações específicas
- Ainda há contenção na linha de cache
- Não funciona para código complexo
- Pode ter problemas de false sharing

**Overhead Estimado:** ~10-50 ciclos de CPU

### 3. Contadores Privados (Thread-Local Storage)

**Características Técnicas:**
- Cada thread trabalha em variável local (stack ou thread-local)
- Redução manual ao final usando sincronização mínima
- Máximo paralelismo durante fase de cálculo
- Exploita localidade de cache dentro de cada thread

**Padrões de Implementação:**

**Padrão 1: Variáveis Locais com Redução Atômica**
```c
#pragma omp parallel
{
    long long local_count = 0;  // Stack local - sem contenção
    
    #pragma omp for
    for (int i = 0; i < N; i++) {
        // Cálculo intensivo sem sincronização
        if (compute_condition(i)) local_count++;
    }
    
    // Redução única por thread
    #pragma omp atomic
    global_count += local_count;
}
```

**Padrão 2: Array Indexado por Thread ID**
```c
long long thread_counts[MAX_THREADS] = {0};

#pragma omp parallel
{
    int tid = omp_get_thread_num();
    
    #pragma omp for
    for (int i = 0; i < N; i++) {
        if (compute_condition(i)) 
            thread_counts[tid]++;  // Sem sincronização
    }
}

// Redução sequencial final
for (int i = 0; i < nthreads; i++) 
    total += thread_counts[i];
```

**Padrão 3: Estruturas Complexas Privadas**
```c
typedef struct {
    long long count;
    double sum;
    double max_value;
    char padding[64];  // Evita false sharing
} ThreadData;

ThreadData thread_data[MAX_THREADS];

#pragma omp parallel
{
    int tid = omp_get_thread_num();
    ThreadData *my_data = &thread_data[tid];
    
    #pragma omp for
    for (int i = 0; i < N; i++) {
        double value = compute_value(i);
        my_data->count++;
        my_data->sum += value;
        if (value > my_data->max_value) 
            my_data->max_value = value;
    }
}
```

**Otimizações Avançadas:**
- **Padding para evitar false sharing**: Separar dados por linha de cache (64 bytes)
- **Alinhamento de memória**: `__attribute__((aligned(64)))`
- **Thread-local storage**: `__thread` ou `thread_local`

**Vantagens:**
- Overhead mínimo de sincronização (1 operação por thread)
- Excelente escalabilidade linear
- Cache locality ótima dentro de cada thread
- Controle total sobre estrutura de dados

**Desvantagens:**
- Código mais verboso e complexo
- Responsabilidade manual de redução
- Possível false sharing em arrays mal alinhados
- Uso adicional de memória

**Overhead Estimado:** ~1-5 ciclos por operação

### 4. Cláusula `reduction`

**Características Técnicas:**
- Implementação automática de padrão redutor pelo compilador OpenMP
- Otimizações inteligentes baseadas no número de threads e tipo de dados
- Suporte a operações matemáticas e lógicas predefinidas
- Geração automática de código thread-safe eficiente

**Operadores Suportados:**
```c
// Operadores aritméticos
#pragma omp parallel for reduction(+:sum)     // Soma
#pragma omp parallel for reduction(*:product) // Produto
#pragma omp parallel for reduction(-:diff)    // Subtração

// Operadores de comparação
#pragma omp parallel for reduction(max:maximum)
#pragma omp parallel for reduction(min:minimum)

// Operadores lógicos
#pragma omp parallel for reduction(&&:all_true)
#pragma omp parallel for reduction(||:any_true)

// Operadores bitwise
#pragma omp parallel for reduction(&:bitwise_and)
#pragma omp parallel for reduction(|:bitwise_or)
#pragma omp parallel for reduction(^:bitwise_xor)
```

**Implementação Interna pelo Compilador:**
```c
// O que o programador escreve:
int sum = 0;
#pragma omp parallel for reduction(+:sum)
for (int i = 0; i < N; i++) {
    sum += array[i];
}

// O que o compilador gera (conceptualmente):
int sum = 0;
#pragma omp parallel
{
    int sum_private = 0;  // Cópia privada inicializada
    
    #pragma omp for
    for (int i = 0; i < N; i++) {
        sum_private += array[i];  // Operação local
    }
    
    // Redução hierárquica otimizada
    #pragma omp critical
    sum += sum_private;
}
```

**Estratégias de Redução Internas:**

**1. Redução Linear (poucas threads):**
```c
// Thread 0: sum_private_0
// Thread 1: sum_private_1  
// Thread 2: sum_private_2
// Final: sum = sum_private_0 + sum_private_1 + sum_private_2
```

**2. Redução em Árvore (muitas threads):**
```
Fase 1: T0+T1, T2+T3, T4+T5, T6+T7
Fase 2: (T0+T1)+(T2+T3), (T4+T5)+(T6+T7)  
Fase 3: Resultado final
```

**3. Redução SIMD (tipos suportados):**
```c
// Uso de instruções vetoriais para somas
// Intel: _mm_add_pd, _mm256_add_ps
// ARM: vaddq_f32
```

**Redução Customizada (OpenMP 4.0+):**
```c
// Definir redutor customizado
#pragma omp declare reduction(complex_add : ComplexNumber : \
    omp_out.real += omp_in.real, omp_out.imag += omp_in.imag) \
    initializer(omp_priv = {0.0, 0.0})

ComplexNumber result = {0.0, 0.0};
#pragma omp parallel for reduction(complex_add:result)
for (int i = 0; i < N; i++) {
    result = complex_multiply(result, data[i]);
}
```

**Otimizações do Compilador:**
- **Vectorização automática**: Uso de SIMD quando possível
- **Redução hierárquica**: Minimiza contenção entre threads
- **Cache-friendly**: Acesso sequencial otimizado
- **Type-specific**: Otimizações por tipo de dados

**Vantagens:**
- Código limpo e legível (uma linha)
- Performance altamente otimizada pelo compilador
- Menos propenso a erros de programação
- Suporte automático a diferentes arquiteturas

**Desvantagens:**
- Limitado a operações predefinidas (até OpenMP 4.0)
- Menos controle sobre implementação específica
- Pode não estar disponível em versões muito antigas
- Debugging mais difícil (código gerado automaticamente)

**Overhead Estimado:** ~5-20 ciclos por operação (otimizado)

### 5. Regiões Críticas Nomeadas (`#pragma omp critical(name)`)

**Características Técnicas:**
- Múltiplos locks independentes identificados por nome
- Permite paralelismo entre recursos diferentes
- Granularidade de sincronização configurável
- Compilador gera locks separados para cada nome

**Sintaxe e Uso:**
```c
FILE *file1, *file2;
int counter1 = 0, counter2 = 0;

#pragma omp parallel
{
    // Estas duas seções podem executar simultaneamente
    #pragma omp critical(output1)
    {
        fprintf(file1, "Thread %d\n", omp_get_thread_num());
        counter1++;
    }
    
    #pragma omp critical(output2)  // Lock diferente!
    {
        fprintf(file2, "Thread %d\n", omp_get_thread_num());
        counter2++;
    }
}
```

**Implementação Interna:**
```c
// O compilador gera algo como:
static omp_lock_t __critical_lock_output1__;
static omp_lock_t __critical_lock_output2__;

// Para critical(output1):
omp_set_lock(&__critical_lock_output1__);
// código protegido
omp_unset_lock(&__critical_lock_output1__);
```

**Casos de Uso Avançados:**
```c
// Processamento de múltiplos arquivos
#pragma omp parallel for
for (int i = 0; i < num_files; i++) {
    process_file_data(i);
    
    // Cada arquivo tem seu próprio lock
    #pragma omp critical(file_output)
    {
        write_results_to_file(i, results[i]);
    }
    
    #pragma omp critical(statistics)  // Diferente do anterior
    {
        update_global_statistics(results[i]);
    }
}
```

**Vantagens:**
- Paralelismo entre recursos independentes
- Granularidade configurável de sincronização
- Melhor escalabilidade que critical global
- Fácil de usar e entender

**Desvantagens:**
- Ainda há serialização dentro de cada grupo
- Número fixo de grupos (compile-time)
- Não escalável dinamicamente
- Possibilidade de deadlock entre grupos

### 6. Locks Explícitos (`omp_lock_t`)

**Características Técnicas:**
- Controle manual completo sobre sincronização
- Locks podem ser criados/destruídos dinamicamente
- Suporte a try-lock (não bloqueante)
- Implementação flexível para casos complexos

**API Completa:**
```c
#include <omp.h>

// Tipos de locks
omp_lock_t simple_lock;        // Lock simples
omp_nest_lock_t nested_lock;   // Lock recursivo

// Operações básicas
void omp_init_lock(omp_lock_t *lock);
void omp_destroy_lock(omp_lock_t *lock);
void omp_set_lock(omp_lock_t *lock);      // Adquire (bloqueia)
void omp_unset_lock(omp_lock_t *lock);    // Libera
int omp_test_lock(omp_lock_t *lock);      // Tenta adquirir

// Locks aninhados (nested)
void omp_init_nest_lock(omp_nest_lock_t *lock);
void omp_destroy_nest_lock(omp_nest_lock_t *lock);
void omp_set_nest_lock(omp_nest_lock_t *lock);
void omp_unset_nest_lock(omp_nest_lock_t *lock);
int omp_test_nest_lock(omp_nest_lock_t *lock);
```

**Exemplo Avançado - Múltiplas Estruturas de Dados:**
```c
typedef struct {
    int *data;
    int size;
    omp_lock_t lock;  // Lock por estrutura
} SafeArray;

SafeArray arrays[NUM_ARRAYS];

// Inicialização
for (int i = 0; i < NUM_ARRAYS; i++) {
    omp_init_lock(&arrays[i].lock);
    arrays[i].data = malloc(1000 * sizeof(int));
    arrays[i].size = 0;
}

#pragma omp parallel
{
    for (int iter = 0; iter < 1000; iter++) {
        int target = rand() % NUM_ARRAYS;
        
        // Lock específico para o array escolhido
        omp_set_lock(&arrays[target].lock);
        
        // Operação thread-safe
        arrays[target].data[arrays[target].size++] = compute_value();
        
        omp_unset_lock(&arrays[target].lock);
    }
}
```

**Try-Lock Pattern (Non-blocking):**
```c
#pragma omp parallel
{
    while (work_available()) {
        omp_lock_t *chosen_lock = NULL;
        
        // Tenta vários recursos até conseguir um
        for (int i = 0; i < num_resources; i++) {
            if (omp_test_lock(&resource_locks[i])) {
                chosen_lock = &resource_locks[i];
                break;
            }
        }
        
        if (chosen_lock) {
            // Trabalha com recurso disponível
            process_resource(i);
            omp_unset_lock(chosen_lock);
        } else {
            // Fazer outro trabalho ou esperar
            do_other_work();
        }
    }
}
```

**Vantagens:**
- Controle completo sobre comportamento de sincronização
- Escalabilidade dinâmica (runtime)
- Suporte a padrões complexos (try-lock, timeouts)
- Máxima flexibilidade

**Desvantagens:**
- Código mais complexo e verboso
- Responsabilidade manual de gerenciamento
- Maior chance de bugs (deadlocks, leaks)
- Debugging mais difícil

### Comparação de Overhead Real

| Mecanismo | Ciclos CPU | Escalabilidade | Flexibilidade | Complexidade |
|-----------|------------|----------------|---------------|---------------|
| `critical` | 100-1000 | ⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| `atomic` | 10-50 | ⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐ |
| Privados | 1-5 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ |
| `reduction` | 5-20 | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐⭐ |
| Critical Named | 100-1000 | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| Locks Explícitos | 50-200 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐ |

## Roteiro para Escolha do Mecanismo de Sincronização

### 🎯 **Guia de Decisão Rápida**

#### 1. **Use `reduction` quando:**
- ✅ Operação é uma redução padrão (+, *, max, min, etc.)
- ✅ Quer código limpo e legível
- ✅ Performance é importante
- ✅ OpenMP 2.0+ disponível

#### 2. **Use contadores privados quando:**
- ✅ `reduction` não suporta sua operação
- ✅ Precisa de controle total sobre redução
- ✅ Debugging/profiling detalhado necessário
- ✅ Operação complexa não-padrão

#### 3. **Use `#pragma omp atomic` quando:**
- ✅ Operação simples (++, +=, -=, *=, etc.)
- ✅ `reduction` não disponível/aplicável
- ✅ Necessário acesso concorrente frequente
- ✅ Performance moderada aceitável

#### 4. **Use `#pragma omp critical` quando:**
- ✅ Código complexo multi-instrução
- ✅ Operações não-atômicas
- ✅ I/O ou chamadas de sistema
- ✅ Performance não é prioridade

#### 5. **Use critical nomeadas quando:**
- ✅ Múltiplas regiões críticas independentes
- ✅ Recursos diferentes que não interferem
- ✅ Granularidade de lock específica

#### 6. **Use locks explícitos quando:**
- ✅ Número de recursos determinado em runtime
- ✅ Estruturas de dados complexas
- ✅ Controle fino sobre sincronização
- ✅ Escalabilidade dinâmica necessária

### 📊 **Matriz de Decisão por Critério**

| Critério | Reduction | Privados | Atomic | Critical | Critical Named | Locks |
|----------|-----------|----------|--------|----------|----------------|-------|
| **Performance** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐ | ⭐⭐ | ⭐⭐⭐⭐ |
| **Simplicidade** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐ |
| **Flexibilidade** | ⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Escalabilidade** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Debugging** | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ |

### 🏗️ **Casos de Uso Específicos**

#### **Computação Científica**
1. **Reduction**: Somas, produtos, máximos
2. **Privados**: Algoritmos complexos customizados
3. **Locks**: Estruturas de dados dinâmicas

#### **Processamento de Dados**
1. **Atomic**: Contadores simples, histogramas
2. **Critical**: I/O, logging
3. **Critical Named**: Múltiplos arquivos/recursos

#### **Simulações**
1. **Privados**: Estados locais complexos
2. **Locks**: Estruturas compartilhadas dinâmicas
3. **Reduction**: Agregação de resultados

#### **Otimização Progressiva**
1. **Início**: Critical (funcionalidade)
2. **Refinamento**: Atomic (performance básica)
3. **Otimização**: Privados/Reduction (máxima performance)
4. **Especialização**: Locks (casos complexos)

## Reflexão sobre Desempenho e Produtividade

### Desempenho
- **Critical**: Gargalo severo - evitar para hot paths
- **Atomic**: Compromisso razoável - 3x melhor que critical
- **Privados/Reduction**: Excelente - 30x+ melhoria

### Produtividade
- **Reduction**: Máxima - código limpo, performance ótima
- **Atomic**: Alta - simplicidade com performance decente
- **Privados**: Média - código mais complexo, mas flexível
- **Critical**: Baixa para performance, alta para funcionalidade

### Recomendação Geral
1. **Primeiro**: Tente `reduction`
2. **Se não der**: Use contadores privados
3. **Para casos específicos**: `atomic` ou `critical`
4. **Para sistemas complexos**: Locks explícitos

## Compilação e Execução

```bash
gcc -fopenmp -O2 -o tarefa10 tarefa10.c
./tarefa10 [num_pontos] [num_threads]

# Exemplo
./tarefa10 100000000 4
```

## Conclusão

A escolha do mecanismo de sincronização deve equilibrar:
- **Performance**: Reduction/Privados > Atomic > Critical
- **Simplicidade**: Reduction > Atomic > Critical > Privados
- **Flexibilidade**: Locks > Critical > Privados > Atomic > Reduction

Para aplicações científicas, privilegie `reduction` quando possível, contadores privados para casos complexos, e reserve `critical` apenas para código não-paralelizável.
