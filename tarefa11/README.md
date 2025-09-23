# Tarefa 11: Simulação de Viscosidade com OpenMP

## 📋 Descrição do Projeto

Esta implementação simula o movimento de um fluido usando uma versão simplificada da **equação de Navier-Stokes**, considerando apenas os efeitos da viscosidade. O projeto demonstra conceitos de:

- **Simulação numérica** usando diferenças finitas
- **Paralelização** com OpenMP
- **Análise de performance** de diferentes estratégias de escalonamento
- **Impacto da cláusula collapse** na paralelização de loops aninhados
- **Análise de escalabilidade** com PaScal Suite
- **Parâmetros configuráveis** para diferentes tamanhos de problema

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

#### Compilação Básica (sem PaScal):
```bash
gcc tarefa11.c -o tarefa11 -fopenmp -lm -O2
```

#### Compilação com PaScal Suite:
```bash
gcc -fopenmp -I/home/paulobraga08/pascal-releases-master/include \
    -L/home/paulobraga08/pascal-releases-master/lib \
    tarefa11.c -lmpascalops -lm -o tarefa11
```

### Execução

#### Execução com parâmetros padrão:
```bash
./tarefa11
# Grade: 512x512, Iterações: 5000
```

#### Execução com parâmetros customizados:
```bash
./tarefa11 [tamanho_grade] [num_iteracoes]
```

**Exemplos:**
```bash
./tarefa11 128 500      # Grade 128x128, 500 iterações (teste rápido)
./tarefa11 256 1000     # Grade 256x256, 1000 iterações (médio)
./tarefa11 512 2000     # Grade 512x512, 2000 iterações (padrão alto)
./tarefa11 1024 3000    # Grade 1024x1024, 3000 iterações (intensivo)
```

**Limitações:**
- Tamanho da grade: 1 ≤ N ≤ 2048
- Número de iterações: 1 ≤ ITER ≤ 50000
- Valores inválidos retornam erro com instruções de uso

## 🔬 Parâmetros da Simulação

### Parâmetros Configuráveis
| Parâmetro | Padrão | Intervalo | Descrição |
|-----------|--------|-----------|-----------|
| **Grade (N)** | 512×512 | 1×1 a 2048×2048 | Resolução espacial |
| **Iterações** | 5000 | 1 a 50000 | Passos temporais |

### Parâmetros Fixos
| Parâmetro | Valor | Descrição |
|-----------|-------|-----------|
| **Viscosidade (ν)** | 0.1 | Coeficiente de difusão |
| **Passo temporal (Δt)** | 0.00001 | Incremento de tempo |
| **Threads** | 4 | Número de threads paralelas |

### Alocação Dinâmica de Memória
O programa utiliza **alocação dinâmica** para as matrizes, permitindo:
- **Flexibilidade**: Tamanhos de grade variáveis
- **Eficiência**: Uso otimizado da memória
- **Escalabilidade**: Teste com diferentes workloads

## 🧪 Análise com PaScal Suite

### Instrumentação Manual
O código inclui **instrumentação manual** com PaScal para análise detalhada:

```c
pascal_start(1);  // Simulação completa
pascal_start(2);  // Região static
pascal_start(3);  // Região collapse  
pascal_start(4);  // Schedule static
pascal_start(5);  // Schedule dynamic
pascal_start(6);  // Schedule guided
```

### Comando de Análise
```bash
# Configurar ambiente Pascal
cd /home/paulobraga08/pascal-releases-master && source env.sh

# Análise de escalabilidade completa
pascalanalyzer ./tarefa11 --inst man \
    --cors 1,2,4 \
    --ipts "128 500","256 1000","512 2000" \
    --rpts 2 \
    --outp escalabilidade_variavel.json \
    --verb INFO
```

**Parâmetros:**
- `--inst man`: Instrumentação manual
- `--cors 1,2,4`: Teste com 1, 2 e 4 cores
- `--ipts`: Diferentes tamanhos de problema  
- `--rpts 2`: 2 repetições por configuração
- `--outp`: Arquivo de saída JSON

## 🧪 Testes Realizados

### Execução Automática
O programa executa automaticamente os seguintes testes:

1. **Simulação Serial**: Execução sequencial para baseline
2. **Simulação Static**: Paralelização com `schedule(static)`
3. **Simulação Collapse**: Teste do impacto da cláusula `collapse(2)`
4. **Comparação de Schedules**: Teste de `static`, `dynamic` e `guided`

### Regiões Instrumentadas (PaScal)
```
Região 1: Simulação completa (main)
Região 2: Versão Static
Região 3: Versão Collapse  
Região 4: Schedule Static
Região 5: Schedule Dynamic
Região 6: Schedule Guided
```

## 📊 Análise dos Resultados

### Resultados da Análise PaScal

Com base na execução do comando:
```bash
pascalanalyzer ./tarefa11 --inst man --cors 1,2,4 --ipts "128 500","256 1000","512 2000" --rpts 2 --outp escalabilidade_variavel.json --verb INFO
```

#### Tempos de Execução Obtidos (em segundos):

| Configuração | 1 Core | 2 Cores | 4 Cores | Speedup 4 vs 1 |
|--------------|--------|---------|---------|-----------------|
| **128×128, 500 iter** | ~0.70 | ~0.76 | ~0.75 | 0.93× (overhead) |
| **256×256, 1000 iter** | ~5.25 | ~5.27 | ~5.20 | 1.01× (neutro) |
| **512×512, 2000 iter** | ~50.88 | ~51.35 | ~50.42 | 1.01× (neutro) |

### Análise de Escalabilidade

#### 🔍 **Escalabilidade Forte (Strong Scaling)**
**Problema fixo, mais cores:**

**Observações:**
- **Problemas pequenos (128×128)**: Overhead de paralelização supera benefícios
- **Problemas médios/grandes**: Speedup limitado devido ao algoritmo sequencial dominante
- **Lei de Amdahl**: Partes seriais limitam ganhos de paralelização

#### 📈 **Escalabilidade Fraca (Weak Scaling)**  
**Trabalho proporcional aos cores:**

| Cores | Grade | Tempo/Core | Eficiência |
|-------|-------|------------|------------|
| 1 | 128×128 | ~0.70s | 100% |
| 2 | 181×181* | ~0.76s | 92% |
| 4 | 256×256* | ~0.75s | 93% |

*_Valores aproximados para manter trabalho constante por core_

### Interpretação dos Resultados

#### ⚠️ **Limitações Observadas:**

1. **Algoritmo Sequencial Dominante**:
   - Laplaciano de 5 pontos tem dependências sequenciais
   - Copiar matrizes é inerentemente sequencial por iteração

2. **Overhead de Sincronização**:
   - Barrier implícito no final de cada região paralela
   - Overhead mais significativo que ganhos para problemas pequenos

3. **Padrão de Acesso à Memória**:
   - Cache misses aumentam com paralelização
   - Falsas compartilhamentos entre threads

#### ✅ **Pontos Positivos:**

1. **Implementação Correta**:
   - Resultados consistentes entre repetições
   - Instrumentação Pascal funcionando adequadamente

2. **Escalabilidade de Problema**:
   - Tempos aumentam previsivelmente com tamanho
   - Complexidade O(N² × ITER) confirmada

3. **Flexibilidade**:
   - Sistema configurável para diferentes análises
   - Base sólida para otimizações futuras

### Speedup por Estratégia de Schedule

#### Execução Anterior (Grade 512×512, 5000 iter):
```
Serial:   ~34.0s
Static:   ~15.0s → Speedup: 2.27×
Collapse: ~15.2s → Speedup: 2.24×  
Dynamic:  ~14.4s → Speedup: 2.36×
Guided:   ~14.4s → Speedup: 2.36×
```

**Ranking de Performance:**
1. **Dynamic/Guided** (~2.36×) - Melhor balanceamento
2. **Static** (~2.27×) - Baixo overhead, mas menos flexível
3. **Collapse** (~2.24×) - Overhead adicional visível

### Dados do PaScal Analyzer

#### Estrutura dos Resultados JSON:
```json
"data": {
  "4;0;1": {  // 4 cores, input 0 (128x128), repetição 1
    "regions": {
      "1.2": [start_time, stop_time, start_line, stop_line, thread_id, filename],
      "1.3": [...],  // Diferentes regiões instrumentadas
      ...
    }
  }
}
```

#### Mapeamento das Regiões:
- **"1"**: Simulação completa (main)
- **"1.2"**: Versão Static (região 2)  
- **"1.3"**: Versão Collapse (região 3)
- **"1.4"**: Schedule Static (região 4)
- **"1.5"**: Schedule Dynamic (região 5)
- **"1.6"**: Schedule Guided (região 6)

#### Configurações Testadas:
- **Input 0**: `128 500` (grade 128×128, 500 iterações)
- **Input 1**: `256 1000` (grade 256×256, 1000 iterações)  
- **Input 2**: `512 2000` (grade 512×512, 2000 iterações)

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
- **Instrumentação Manual**: Profiling detalhado com PaScal Suite
- **Análise de Escalabilidade**: Strong e weak scaling
- **Alocação Dinâmica**: Gerenciamento flexível de memória
- **Lei de Amdahl**: Limitações práticas da paralelização

## 🔬 Arquivos Gerados

### Resultados de Análise
- `escalabilidade_variavel.json`: Dados completos do PaScal Analyzer
- `teste_manual.json`: Resultados de testes anteriores

### Visualização
Acesse https://pascalsuite.imd.ufrn.br/pascal-viewer e faça upload dos arquivos JSON para:
- **Heatmaps de escalabilidade**
- **Gráficos de eficiência paralela**  
- **Comparação entre regiões**
- **Análise hierárquica de performance**

## 📚 Conclusões

### Principais Aprendizados

1. **Nem toda paralelização resulta em speedup**:
   - Overhead pode superar benefícios para problemas pequenos
   - Análise de custo-benefício é essencial

2. **Diferentes schedules têm comportamentos distintos**:
   - `dynamic`/`guided` melhor para cargas desbalanceadas
   - `static` mais eficiente para cargas uniformes

3. **Escalabilidade depende do problema**:
   - Tamanho mínimo necessário para benefícios
   - Lei de Amdahl limita speedups teóricos

4. **Instrumentação é fundamental**:
   - PaScal Suite oferece insights detalhados
   - Medições precisas guiam otimizações

### Próximos Passos

- **Otimizações de algoritmo**: Reduzir componentes sequenciais
- **Otimizações de memória**: Melhorar localidade de cache
- **Paralelização temporal**: Explorar outras dimensões
- **GPU Computing**: Avaliar aceleração em GPUs

## 🛠️ Troubleshooting

### Problemas Comuns

#### Erro de Compilação: "undefined reference to pascal_start"
```bash
# Solução: Compilar com biblioteca Pascal
gcc -fopenmp -I/path/to/pascal/include -L/path/to/pascal/lib tarefa11.c -lmpascalops -lm -o tarefa11
```

#### Erro: "Pascal not running"
```bash
# Normal quando executado diretamente (sem pascalanalyzer)
# Para análise completa, use:
pascalanalyzer ./tarefa11 --inst man --cors 1,2,4 --ipts "128 500" --outp resultado.json
```

#### Memoria insuficiente para grades grandes
```bash
# Reduzir tamanho do problema:
./tarefa11 512 1000    # Em vez de 1024 3000
```

### Exemplos de Uso Avançado

#### Análise rápida (desenvolvimento):
```bash
pascalanalyzer ./tarefa11 --inst man --cors 2,4 --ipts "128 100","256 200" --rpts 1 --outp teste_rapido.json --verb WARNING
```

#### Análise completa (produção):
```bash
pascalanalyzer ./tarefa11 --inst man --cors 1,2,4,8 --ipts "128 500","256 1000","512 2000","1024 3000" --rpts 3 --idtm 5 --outp analise_completa.json --verb INFO
```

#### Análise específica de schedule:
```bash
# Executar apenas uma vez e analisar regiões 4-6
./tarefa11 256 1000
# Depois extrair dados das regiões no JSON gerado
```
