# Tarefa 3 - Aproximação de π com Análise de Performance

## Descrição da Tarefa

Este projeto implementa um programa em C que calcula aproximações de π usando séries matemáticas, variando o número de iterações e medindo o tempo de execução. O objetivo é comparar os valores obtidos com o valor real de π e analisar como a acurácia melhora com mais processamento computacional.

## Teoria Matemática

### 1. Série de Leibniz

A **Série de Leibniz** é uma das fórmulas mais conhecidas para calcular π:

```
π/4 = 1 - 1/3 + 1/5 - 1/7 + 1/9 - 1/11 + ...
```

**Características:**

- Convergência lenta (O(1/n))
- Simples de implementar
- Requer muitas iterações para alta precisão
- Erro aproximado: |erro| ≈ 1/(2n+1)

### 2. Série de Nilakantha

A **Série de Nilakantha** oferece convergência mais rápida:

```
π = 3 + 4/(2×3×4) - 4/(4×5×6) + 4/(6×7×8) - ...
```

**Características:**

- Convergência mais rápida que Leibniz
- Baseada em produtos consecutivos
- Melhor precisão com menos iterações
- Erro aproximado: |erro| ≈ 1/n³

## Funcionalidades Implementadas

### Algoritmos de Cálculo

- **calculate_pi_leibniz()**: Implementa a série de Leibniz
- **calculate_pi_nilakantha()**: Implementa a série de Nilakantha

### Análise de Performance

- **measure_time()**: Mede tempo de execução com precision de clock
- **calculate_error()**: Calcula erro absoluto comparado ao valor real
- **print_results()**: Formata resultados em tabela organizada

### Testes Automatizados

- Testa com 6 diferentes números de iterações (100 a 10,000,000)
- Compara performance entre os dois métodos
- Análise comparativa detalhada

## Estrutura do Código

```c
// Constantes
#define PI_REAL 3.14159265358979323846

// Funções principais
double calculate_pi_leibniz(long long iterations);
double calculate_pi_nilakantha(long long iterations);
double measure_time(double (*func)(long long), long long iterations);
double calculate_error(double approximation);
void print_results(const char* method, long long iterations,
                   double pi_approx, double time_taken);
```

## Resultados Esperados

### Tabela de Performance

O programa gera uma tabela comparativa mostrando:

- Método utilizado
- Número de iterações
- Valor de π aproximado
- Tempo de execução (segundos)
- Erro absoluto
- Percentual de precisão

### Exemplo de Saída

```
Método          |   Iterações |    π Aproximado |  Tempo (s) |         Erro |  Precisão
-----------------|-------------|-----------------|------------|--------------|----------
Leibniz         |        1000 | 3.140592653590  |   0.000015 |     1.00e-03 |  99.9682%
Nilakantha      |        1000 | 3.141592653590  |   0.000012 |     1.00e-08 |  99.9999%
```

## Análise Teórica dos Resultados

### 1. Convergência

- **Leibniz**: Convergência O(1/n) - lenta
- **Nilakantha**: Convergência O(1/n³) - rápida

### 2. Complexidade Temporal

- Ambos algoritmos: O(n) onde n = número de iterações
- Tempo cresce linearmente com iterações

### 3. Precisão vs Performance

- Mais iterações = maior precisão
- Lei dos retornos decrescentes aplicada
- Trade-off entre tempo e acurácia

## Aplicações em Computação Real

### 1. Simulações Científicas

- **CFD**: Malhas mais finas requerem mais iterações
- **Elementos Finitos**: Precisão vs custo computacional
- **Modelagem Climática**: Milhões de cálculos iterativos

### 2. Inteligência Artificial

- **Treinamento de Redes Neurais**: Epochs vs overfitting
- **Convergência de Algoritmos**: Critérios de parada
- **Otimização**: Gradiente descendente iterativo

### 3. Computação Financeira

- **Monte Carlo**: Simulações para precificação
- **Análise de Risco**: Precisão vs tempo de resposta
- **High-Frequency Trading**: Latência crítica

### 4. Estratégias de Otimização

- **Paralelização**: Distribuir iterações entre cores
- **Convergência Adaptativa**: Parar quando precisão suficiente
- **Hardware Especializado**: GPUs, FPGAs, TPUs

## Conceitos de Programação Paralela

Este projeto demonstra conceitos fundamentais para programação paralela:

### 1. **Paralelização Potencial**

- Loop principal pode ser dividido entre threads
- Redução paralela para somar termos
- Independência entre iterações

### 2. **Medição de Performance**

- Benchmarking preciso com clock()
- Análise de escalabilidade
- Profiling de algoritmos

### 3. **Trade-offs Computacionais**

- Tempo vs Precisão
- Memória vs Velocidade
- Algoritmo vs Hardware

## O Padrão de Precisão Crescente em Aplicações Reais

O comportamento observado neste projeto - onde maior esforço computacional resulta em precisão incrementalmente melhor - é um padrão fundamental que se repete em diversas aplicações críticas da computação moderna.

### 1. **Simulações Físicas de Alta Fidelidade**

#### **Dinâmica de Fluidos Computacional (CFD)**

- **Aeronáutica**: Simulação de fluxo de ar sobre asas de aeronaves
  - 10³ células: estimativa grosseira do arrasto
  - 10⁶ células: captura turbulência básica
  - 10⁹ células: resolve detalhes críticos para segurança
  - **Custo**: Dias de supercomputador para cada incremento de precisão
  - **Impacto**: Diferença entre aprovação e rejeição em certificação

#### **Simulações Estruturais (Elementos Finitos)**

- **Engenharia Civil**: Análise de pontes e edifícios
  - Malha grosseira: tendências gerais de tensão
  - Malha refinada: identifica pontos de falha críticos
  - **Trade-off**: Cada refinamento dobra o tempo de computação
  - **Consequência**: Erro de 1% pode significar colapso estrutural

#### **Modelagem Climática**

- **Previsão Meteorológica**: Modelos globais do clima
  - Resolução 100km: previsão regional básica
  - Resolução 10km: captura sistemas locais
  - Resolução 1km: resolve convecção detalhada
  - **Realidade**: Modelos do ECMWF usam milhares de processadores por semanas

### 2. **Inteligência Artificial e Aprendizado de Máquina**

#### **Treinamento de Modelos de Linguagem**

- **GPT Evolution**: Demonstração clara do padrão precisão vs recursos
  - GPT-1 (117M parâmetros): texto básico
  - GPT-3 (175B parâmetros): capacidades emergentes
  - GPT-4 (~1.7T parâmetros): raciocínio sofisticado
  - **Custo**: Crescimento exponencial de recursos (energia, tempo, hardware)

#### **Redes Neurais Convolucionais**

- **Visão Computacional**: Reconhecimento de imagens médicas
  - 10 epochs: detecção grosseira de anomalias
  - 100 epochs: classificação confiável
  - 1000 epochs: detecção de detalhes sutis críticos para diagnóstico
  - **Dilema**: Overfitting vs underfitting - encontrar o ponto ótimo

#### **Simulações de Monte Carlo em IA**

- **Algoritmos de Busca**: AlphaGo e sucessores
  - 1000 simulações: jogada razoável
  - 100.000 simulações: nível profissional
  - 10.000.000 simulações: superhumano
  - **Escalabilidade**: Cada ordem de magnitude requer 10x mais hardware

### 3. **Computação Científica e Pesquisa**

#### **Descoberta de Medicamentos**

- **Dinâmica Molecular**: Simulação de proteínas
  - Nanosegundos: movimentos locais
  - Microsegundos: mudanças conformacionais
  - Milisegundos: dobramento completo de proteínas
  - **Desafio**: Cada escala temporal requer ordens de magnitude mais computação

#### **Física de Partículas**

- **LHC (Large Hadron Collider)**: Análise de colisões
  - 10⁶ eventos: tendências estatísticas básicas
  - 10⁹ eventos: descoberta de partículas conhecidas
  - 10¹² eventos: evidência de nova física
  - **Infraestrutura**: Grid computacional mundial processando petabytes

#### **Astrofísica Computacional**

- **Simulações N-Body**: Formação de galáxias
  - 10⁶ partículas: estrutura geral
  - 10⁹ partículas: subestruturas detalhadas
  - 10¹² partículas: resolução de halos de matéria escura
  - **Recursos**: Supercomputadores exascale em desenvolvimento

### 4. **Aplicações Financeiras de Alto Risco**

#### **Modelos de Risco Quantitativo**

- **Monte Carlo para Derivativos**: Precificação de opções complexas
  - 10⁴ simulações: estimativa inicial
  - 10⁶ simulações: precisão comercial
  - 10⁸ simulações: precisão regulatória
  - **Consequência**: Erro de precificação = perdas milhonárias

#### **High-Frequency Trading**

- **Algoritmos de Latência Ultra-Baixa**
  - Precisão vs velocidade: microsegundos importam
  - Trade-off: algoritmo simples (rápido) vs complexo (preciso)
  - **Realidade**: Diferença de 1 microsegundo = vantagem competitiva

### 5. **Padrões Comuns e Lições Aprendidas**

#### **Lei dos Retornos Decrescentes Universais**

```
Precisão ∝ log(Recursos Computacionais)
```

- **10x mais recursos** → **~2-3x mais precisão**
- **100x mais recursos** → **~4-5x mais precisão**
- Padrão consistente entre domínios diferentes

#### **Estratégias de Otimização Emergentes**

1. **Computação Adaptativa**

   - Alocar recursos onde precisão é mais crítica
   - Refinamento hierárquico de malhas
   - Early stopping em ML quando convergência adequada

2. **Aproximações Inteligentes**

   - Modelos substitutos (surrogate models)
   - Reduced-order modeling
   - Transfer learning para reutilizar computação

3. **Hardware Especializado**
   - TPUs para AI
   - FPGAs para simulações específicas
   - Computação quântica para problemas específicos

#### **Implicações para o Futuro**

- **Sustentabilidade**: Crescimento exponencial de demanda energética
- **Democratização**: Acesso a computação de alta performance
- **Algoritmos**: Necessidade de métodos mais eficientes
- **Hardware**: Especializações para domínios específicos

## Conclusões

Este projeto ilustra princípios fundamentais da computação científica:

1. **Algoritmos diferentes têm características de convergência distintas**
2. **Existe sempre um trade-off entre precisão e performance**
3. **A escolha do algoritmo pode ser mais importante que recursos computacionais**
4. **Medição rigorosa é essencial para otimização**

Estes conceitos são aplicáveis em simulações reais, IA, computação financeira e qualquer área que demande cálculos iterativos de alta precisão.
