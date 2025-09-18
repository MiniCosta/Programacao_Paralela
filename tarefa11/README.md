# Tarefa 11: Simulação de Viscosidade com OpenMP

## 📋 Descrição do Projeto

Esta implementação simula o movimento de um fluido usando uma versão simplificada da **equação de Navier-Stokes**, considerando apenas os efeitos da viscosidade. O projeto demonstra conceitos de:

- **Simulação numérica** usando diferenças finitas
- **Paralelização** com OpenMP
- **Análise de performance** de diferentes estratégias de escalonamento
- **Impacto da cláusula collapse** na paralelização de loops aninhados

### Equação Implementada
```
∂u/∂t = ν∇²u
∂v/∂t = ν∇²v
```
Onde:
- `u, v`: componentes da velocidade nas direções x e y
- `ν`: viscosidade cinemática (0.1)
- `∇²`: operador laplaciano (difusão)

## 🚀 Compilação e Execução

### Compilação
```bash
gcc tarefa11.c -o tarefa11 -fopenmp -lm -O2
```

### Execução

#### Execução com número padrão de threads (4):
```bash
./tarefa11
```

#### Execução com número específico de threads:
```bash
./tarefa11 [número_de_threads]
```

**Exemplos:**
```bash
./tarefa11 1    # Executa com 1 thread (força serial)
./tarefa11 2    # Executa com 2 threads
./tarefa11 4    # Executa com 4 threads (padrão)
./tarefa11 8    # Executa com 8 threads
```

**Limitações:**
- Número de threads deve estar entre 1 e 8
- Valores inválidos retornam erro com instruções de uso

## 🔬 Parâmetros da Simulação

| Parâmetro | Valor | Descrição |
|-----------|-------|-----------|
| **Grade** | 512x512 | Resolução espacial |
| **Iterações** | 5000 | Passos temporais |
| **Viscosidade (ν)** | 0.1 | Coeficiente de difusão |
| **Passo temporal (Δt)** | 0.00001 | Incremento de tempo |

## 🧪 Testes Realizados

O programa executa automaticamente os seguintes testes:

1. **Simulação Serial**: Execução sequencial para baseline
2. **Simulação Static**: Paralelização com `schedule(static)`
3. **Simulação Collapse**: Teste do impacto da cláusula `collapse(2)`
4. **Comparação de Schedules**: Teste de `static`, `dynamic` e `guided`

## 📊 Análise dos Resultados

### Speedup Calculado
Com base na execução realizada com 4 cores:

```
Simulação Serial:     ~33.8s
Static:   ~15.0s → Speedup: ~2.25x
Collapse: ~15.2s → Speedup: ~2.22x
Dynamic:  ~14.4s → Speedup: ~2.35x
```

### Interpretação dos Schedules

#### 🔹 **Static (Estática)**
- **Como funciona**: Divisão pré-definida e igual das iterações
- **Vantagens**: Baixo overhead, previsível
- **Melhor para**: Cargas de trabalho uniformes (como nossa simulação)

#### 🔹 **Dynamic (Dinâmica)**  
- **Como funciona**: Distribuição sob demanda em blocos fixos
- **Vantagens**: Adaptável a cargas variáveis
- **Desvantagem**: Maior overhead para problemas regulares

#### 🔹 **Guided (Guiada)**
- **Como funciona**: Blocos de tamanho decrescente
- **Uso**: Equilibrio entre static e dynamic
- **Performance**: Geralmente intermediária

### 🎭 Analogias para os Schedules

Para entender melhor como cada schedule funciona, imagine uma **fábrica com 4 trabalhadores** processando **1000 peças**:

#### 📦 **Static - "Divisão Fixa"**
```
Trabalhador 1: peças 1-250
Trabalhador 2: peças 251-500  
Trabalhador 3: peças 501-750
Trabalhador 4: peças 751-1000
```
**Analogia**: Como dividir uma pizza em fatias iguais - cada pessoa sabe exatamente qual pedaço é seu desde o início.
#### 🏃 **Dynamic - "Fila do Banco"**
```
Fila de tarefas: [chunk1][chunk2][chunk3]...[chunkN]
Trabalhador livre pega próximo chunk da fila
```
**Analogia**: Como um caixa de banco - quando um cliente termina, o próximo da fila é atendido. Funciona bem quando alguns clientes demoram mais (iterações complexas).

#### 📈 **Guided - "Fatias Decrescentes"**
```
1º chunk: 400 peças
2º chunk: 300 peças  
3º chunk: 200 peças
4º chunk: 100 peças
```
**Analogia**: Como comer uma torta começando com fatias grandes e diminuindo conforme fica saciado. Combina a eficiência inicial do static com a flexibilidade final do dynamic.

### Cláusula Collapse

A diretiva `collapse(2)` combina dois loops aninhados em um único espaço de iteração:

```c
#pragma omp parallel for schedule(static) collapse(2)
for (int i = 1; i < N-1; i++) {
    for (int j = 1; j < N-1; j++) {
        // Computação aqui
    }
}
```

**Benefícios:**
- Aumenta o paralelismo disponível
- Melhora balanceamento de carga
- Mais eficiente com muitos threads

**Limitações:**
- Pode adicionar overhead para problemas pequenos
- Nem sempre resulta em speedup

## 📈 Como Interpretar a Saída

### Exemplo de Saída Real:
```
=== SIMULAÇÃO DE VISCOSIDADE - NAVIER-STOKES ===
Grade: 512x512, Iterações: 5000, Viscosidade: 0.100
Número de threads: 4

Estado inicial: perturbação criada no centro

=== VERSÃO 1: SIMULAÇÃO SERIAL ===
Tempo VERSÃO 1 (serial): 33.7629 segundos

=== VERSÃO 2: SIMULAÇÃO STATIC (4 threads) ===  
Tempo VERSÃO 2 (static): 15.0199 segundos

=== VERSÃO 3: SIMULAÇÃO COLLAPSE (4 threads) ===
Tempo VERSÃO 3 (collapse): 15.2336 segundos

=== VERSÕES 4-6: COMPARAÇÃO DE SCHEDULES ===
=== VERSÃO 4: Testando schedule static ===
Tempo VERSÃO 4 (static): 15.1564 segundos
=== VERSÃO 5: Testando schedule dynamic ===
Tempo VERSÃO 5 (dynamic): 14.3926 segundos  
=== VERSÃO 6: Testando schedule guided ===
Tempo VERSÃO 6 (guided): 14.4305 segundos
```

### Análise:
1. **Dynamic** apresenta melhor performance neste caso específico
2. **Static** e **Guided** têm performance similar
3. **Collapse** adiciona pequeno overhead (~1%)
4. **Speedup** de ~2.35x com 4 threads é bom para este problema

## 🎯 Conceitos Demonstrados

- **Simulação de PDE**: Implementação numérica de equações diferenciais
- **Stencil Computations**: Padrão de acesso a vizinhos em grade
- **OpenMP Scheduling**: Diferentes estratégias de distribuição de trabalho
- **Performance Analysis**: Medição e comparação de tempos de execução
- **Paralelização de Loops**: Técnicas para acelerar computação intensiva
