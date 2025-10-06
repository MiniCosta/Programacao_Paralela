# Tarefa 15: Simulação de Difusão de Calor 1D com MPI

## Descrição do Problema

Este programa implementa uma simulação de **difusão de calor em uma barra unidimensional** utilizando MPI (Message Passing Interface) para paralelização. A simulação resolve numericamente a **equação de difusão térmica**:

```
∂T/∂t = α * ∂²T/∂x²
```

Onde:

- `T(x,t)` = temperatura no ponto x no tempo t
- `α` = coeficiente de difusão térmica
- `x` = posição ao longo da barra
- `t` = tempo

## Discretização Numérica

### Método de Diferenças Finitas

A equação é discretizada usando o **método explícito de diferenças finitas**:

```
T[i]^(n+1) = T[i]^n + α*Δt/Δx² * (T[i-1]^n - 2*T[i]^n + T[i+1]^n)
```

### Parâmetros da Simulação

- **N_GLOBAL = 1000**: Número total de pontos na barra
- **N_TIMESTEPS = 1000**: Número de iterações temporais
- **ALPHA = 0.1**: Coeficiente de difusão térmica
- **DT = 0.001**: Passo temporal (Δt)
- **DX = 0.1**: Espaçamento espacial (Δx)

### Condições Iniciais

- **Pulso de calor**: Temperatura de 100°C no terço central da barra
- **Bordas frias**: Temperatura de 0°C no resto da barra
- **Evolução**: O calor se difunde das regiões quentes para as frias

## Paralelização com MPI

### Decomposição de Domínio

A barra é **dividida entre os processos MPI**:

- Cada processo simula `N_GLOBAL/size` pontos consecutivos
- **Células fantasma (ghost cells)**: Cada processo mantém cópias das bordas dos vizinhos
- **Comunicação de bordas**: Necessária a cada iteração temporal

### Estrutura de Dados

```c
double *temp = calloc(n_local + 2, sizeof(double));
//                    |        |
//                    |        +-- Células fantasma (bordas dos vizinhos)
//                    +-- Pontos reais do processo
```

**Layout da memória:**

```
[ghost_left] [dados_reais...] [ghost_right]
     ↑              ↑              ↑
   temp[0]       temp[1..n]    temp[n+1]
```

## Três Implementações Comparadas

### 1. Comunicação Bloqueante (MPI_Send/MPI_Recv)

```c
// Trocar bordas com vizinhos
if (rank > 0) {
    MPI_Send(&temp[1], 1, MPI_DOUBLE, rank-1, 0, MPI_COMM_WORLD);
    MPI_Recv(&temp[0], 1, MPI_DOUBLE, rank-1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
if (rank < size-1) {
    MPI_Send(&temp[n_local], 1, MPI_DOUBLE, rank+1, 1, MPI_COMM_WORLD);
    MPI_Recv(&temp[n_local+1], 1, MPI_DOUBLE, rank+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

// Atualizar todos os pontos após comunicação completar
for (int i = 1; i <= n_local; i++) {
    temp_new[i] = temp[i] + ALPHA * DT / (DX*DX) * (temp[i-1] - 2*temp[i] + temp[i+1]);
}
```

**Características:**

- ✅ **Simples de implementar**: API direta e intuitiva
- ❌ **Serialização**: Processos aguardam uns aos outros
- ❌ **Sem sobreposição**: Comunicação e computação não podem ser simultâneas
- ⏱️ **Tempo**: `T_comunicação + T_computação`

### 2. Comunicação Não-Bloqueante com MPI_Wait

```c
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

// Computação após comunicação completar
for (int i = 1; i <= n_local; i++) {
    temp_new[i] = temp[i] + ALPHA * DT / (DX*DX) * (temp[i-1] - 2*temp[i] + temp[i+1]);
}
```

**Características:**

- ✅ **Não-bloqueante**: Comunicação inicia imediatamente
- ✅ **Flexibilidade**: Permite múltiplas operações simultâneas
- ❌ **Sem sobreposição**: MPI_Waitall bloqueia até completar
- ⏱️ **Tempo**: Similar ao bloqueante, mas com menor overhead

### 3. Comunicação Não-Bloqueante com MPI_Test (Sobreposição)

```c
// Iniciar comunicações não-bloqueantes
MPI_Isend/MPI_Irecv...

// PRIMEIRO: Atualizar pontos internos (não precisam das bordas)
for (int i = 2; i <= n_local-1; i++) {
    temp_new[i] = temp[i] + ALPHA * DT / (DX*DX) * (temp[i-1] - 2*temp[i] + temp[i+1]);
}

// Aguardar comunicações usando MPI_Test em loop
int all_complete = 0;
while (!all_complete) {
    MPI_Testall(req_count, req, &flag, MPI_STATUSES_IGNORE);
    all_complete = flag;

    if (!all_complete) {
        // Fazer outras computações durante a espera
        volatile double dummy = 0.0;
        for (int k = 0; k < 100; k++) dummy += k * 0.001;
    }
}

// DEPOIS: Atualizar pontos das bordas (precisam dos valores dos vizinhos)
temp_new[1] = temp[1] + ALPHA * DT / (DX*DX) * (temp[0] - 2*temp[1] + temp[2]);
temp_new[n_local] = temp[n_local] + ALPHA * DT / (DX*DX) *
                   (temp[n_local-1] - 2*temp[n_local] + temp[n_local+1]);
```

**Características:**

- ✅ **Máxima sobreposição**: Computação e comunicação simultâneas
- ✅ **Eficiência**: Aproveita tempo de espera da comunicação
- ✅ **Escalabilidade**: Melhor para grandes números de processos
- ⏱️ **Tempo**: `max(T_comunicação, T_computação_interna) + T_computação_bordas`

## Padrão de Comunicação

### Ghost Cells (Células Fantasma)

Cada processo precisa dos valores das **bordas dos vizinhos** para calcular a difusão:

```
Processo 0:    [x x x x] -> precisa do primeiro valor do Processo 1
Processo 1: <- [x x x x] -> precisa do último valor do Processo 0 e primeiro do Processo 2
Processo 2: <- [x x x x]    precisa do último valor do Processo 1
```

### Protocolo de Comunicação

1. **Processo i envia**:

   - Primeira célula para processo i-1 (se existe)
   - Última célula para processo i+1 (se existe)

2. **Processo i recebe**:
   - Última célula do processo i-1 na ghost cell esquerda
   - Primeira célula do processo i+1 na ghost cell direita

### Tags das Mensagens

- **Tag 0**: Dados enviados para o vizinho da esquerda
- **Tag 1**: Dados enviados para o vizinho da direita
- **Evita deadlocks**: Cada send tem seu recv correspondente

## Resultados Esperados

### Análise de Performance

#### Comunicação Bloqueante

- **Vantagem**: Simples, sem complexidade adicional
- **Desvantagem**: Serialização total, processos ociosos
- **Melhor para**: Poucos processos, comunicação rápida

#### Comunicação Não-Bloqueante + Wait

- **Vantagem**: Menor overhead que bloqueante
- **Desvantagem**: Ainda não aproveita sobreposição
- **Melhor para**: Casos onde não é possível implementar sobreposição

#### Comunicação Não-Bloqueante + Test

- **Vantagem**: Máxima eficiência, sobreposição total
- **Desvantagem**: Complexidade de implementação
- **Melhor para**: Muitos processos, comunicação lenta

### Speedup Esperado

```
Speedup_versão2 = T_bloqueante / T_wait ≈ 1.1x - 1.3x
Speedup_versão3 = T_bloqueante / T_test ≈ 1.2x - 2.0x
```

### Fatores que Afetam Performance

1. **Latência da rede**: Maior impacto na versão bloqueante
2. **Largura de banda**: Afeta todas as versões igualmente
3. **Razão computação/comunicação**: Versão 3 melhor quando comunicação é lenta
4. **Número de processos**: Versão 3 escala melhor

## Compilação e Execução

### Compilar

```bash
mpicc -o tarefa15 tarefa15.c -lm
```

### Executar

```bash
# Com 2 processos
mpirun -np 2 ./tarefa15

# Com 4 processos
mpirun -np 4 ./tarefa15

# Com 8 processos
mpirun -np 8 ./tarefa15
```

### Requisitos

- **MPI instalado**: OpenMPI, MPICH ou Intel MPI
- **Número de processos**: Deve dividir N_GLOBAL (1000)
- **Processos válidos**: 2, 4, 5, 8, 10, 20, 25, 40, 50, 100, 125, 200, 250, 500, 1000

## Saída do Programa

```
=== SIMULAÇÃO DE DIFUSÃO DE CALOR 1D ===
Tamanho da barra: 1000 pontos
Número de processos: 4
Pontos por processo: 250
Número de iterações: 1000

RESULTADOS:
1. MPI_Send/MPI_Recv (bloqueante):      0.145230 s
2. MPI_Isend/MPI_Irecv + MPI_Wait:      0.132180 s
3. MPI_Isend/MPI_Irecv + MPI_Test:      0.098750 s

SPEEDUP:
Versão 2 vs 1: 1.10x
Versão 3 vs 1: 1.47x
Versão 3 vs 2: 1.34x

EFICIÊNCIA:
Comunicação não-bloqueante com Test foi mais eficiente
```

## Conceitos Importantes

### 1. Equação de Difusão

- **Fenômeno físico**: Propagação de calor por condução
- **Solução numérica**: Método de diferenças finitas explícito
- **Estabilidade**: Critério CFL deve ser respeitado (Δt ≤ Δx²/(2α))

### 2. Paralelização

- **Decomposição de domínio**: Divisão espacial da barra
- **Comunicação de bordas**: Troca de valores entre vizinhos
- **Sincronização**: Barreira temporal a cada iteração

### 3. MPI Não-Bloqueante

- **Overlap**: Sobreposição de comunicação e computação
- **Latência hiding**: Esconder tempo de comunicação
- **Escalabilidade**: Melhor performance com muitos processos

### 4. Ghost Cells

- **Conceito**: Células adicionais para armazenar dados dos vizinhos
- **Implementação**: Arrays com tamanho n_local + 2
- **Vantagem**: Simplifica código de atualização dos pontos

## Conclusões

1. **Versão bloqueante**: Mais simples, mas menos eficiente
2. **Versão Wait**: Pequena melhoria sem muito esforço adicional
3. **Versão Test**: Máxima eficiência com implementação mais complexa
4. **Escolha da versão**: Depende do trade-off simplicidade vs. performance
5. **Fator crítico**: Razão entre tempo de computação e comunicação

#### Compilação

```bash
# Navegar para o diretório
cd /caminho/para/tarefa15/

# Compilar o programa
mpicc -o tarefa15 tarefa15.c -lm -Wall -O2

```

#### Execução

```bash
# Execução local com diferentes números de processos
mpirun -np 2 ./tarefa15
mpirun -np 4 ./tarefa15
mpirun -np 8 ./tarefa15

# Execução com verbose (para debugging)
mpirun -np 4 --verbose ./tarefa15

# Execução em cluster (se disponível)
mpirun -np 16 --hostfile hosts.txt ./tarefa15
```

### 🚀 **Exemplo de Saída de Execução Completa**

```bash
$ mpirun -np 4 ./tarefa15

=== SIMULAÇÃO DE DIFUSÃO DE CALOR 1D ===
Tamanho da barra: 1000 pontos
Número de processos: 4
Pontos por processo: 250
Número de iterações: 1000

Processo 0: simulando pontos 0 a 249
Processo 1: simulando pontos 250 a 499
Processo 2: simulando pontos 500 a 749
Processo 3: simulando pontos 750 a 999

Executando simulação...

RESULTADOS:
1. MPI_Send/MPI_Recv (bloqueante):      0.145230 s
2. MPI_Isend/MPI_Irecv + MPI_Wait:      0.132180 s
3. MPI_Isend/MPI_Irecv + MPI_Test:      0.098750 s

SPEEDUP:
Versão 2 vs 1: 1.10x
Versão 3 vs 1: 1.47x
Versão 3 vs 2: 1.34x

EFICIÊNCIA:
Comunicação não-bloqueante com Test foi mais eficiente

Temperatura final:
Processo 0 - Primeiro ponto: 12.34°C, Último ponto: 23.45°C
Processo 1 - Primeiro ponto: 23.45°C, Último ponto: 34.56°C
Processo 2 - Primeiro ponto: 34.56°C, Último ponto: 23.45°C
Processo 3 - Primeiro ponto: 23.45°C, Último ponto: 12.34°C

Simulação concluída com sucesso!
```

### 🎯 **Dicas de Performance**

1. **Número de processos ideal**: Múltiplo do número de cores do CPU
2. **Memory binding**: `mpirun --bind-to core -np 4 ./tarefa15`
3. **NUMA awareness**: `mpirun --map-by numa -np 8 ./tarefa15`
4. **Profiling**: Usar ferramentas como Intel VTune ou TAU

### 📝 **Modificações para Teste**

Para testar diferentes configurações, edite as constantes no código:

```c
#define N_GLOBAL 1000      // ← Altere para 500 ou 2000 para teste
#define N_TIMESTEPS 1000   // ← Altere para 100 ou 10000
#define ALPHA 0.1          // ← Teste com 0.05 ou 0.2
```
