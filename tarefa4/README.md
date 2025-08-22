## Discussão: Gargalo de von Neumann e Multithreading de Hardware

### Quais programas são limitados pela memória (gargalo de von Neumann) e quais não são?

O chamado "gargalo de von Neumann" ocorre quando a velocidade de transferência de dados entre a memória e a CPU limita o desempenho do programa. Programas que realizam muitas operações de leitura e escrita em grandes volumes de dados, mas com operações aritméticas simples, são chamados de memory-bound. No contexto deste projeto:

- **O programa de soma de vetores (`memoria_limitada`) é memory-bound:**
  Ele é limitado pela taxa com que os dados podem ser lidos da memória e somados. Mesmo com muitos núcleos, o desempenho só melhora até saturar a largura de banda da memória.

- **O programa de cálculos matemáticos intensivos (`cpu_limitada`) é compute-bound:**
  Ele realiza muitos cálculos para cada dado lido, sendo limitado pela capacidade de processamento da CPU, não pela memória.

### Como o multi-thread de hardware pode ajudar (ou atrapalhar)

O multithreading de hardware (como Hyper-Threading da Intel) permite que cada núcleo físico execute múltiplos threads, alternando entre eles para aproveitar melhor os recursos do processador.

- **Em programas memory-bound:**
  O multithreading pode ajudar, pois enquanto um thread espera dados da memória, outro pode ser executado, reduzindo o tempo ocioso do processador e melhorando o uso da largura de banda.

- **Em programas compute-bound:**
  O multithreading pode atrapalhar, pois vários threads competem pelos mesmos recursos de execução (ALUs, FPU, cache), causando contenção e, muitas vezes, diminuindo o desempenho por thread. O ideal é não ultrapassar o número de núcleos físicos para esse tipo de programa.

**Resumo:**
- Memory-bound: gargalo de von Neumann, multithreading pode ajudar até saturar a memória.
- Compute-bound: limitado pela CPU, multithreading de hardware pode atrapalhar se houver competição excessiva por recursos internos do processador.
# Tarefa 4 – Programação Paralela com OpenMP

Este projeto apresenta dois exemplos de paralelização em C usando OpenMP:
- Um programa limitado por memória (memory-bound), que realiza somas simples em vetores.
- Um programa limitado por CPU (compute-bound), que executa cálculos matemáticos intensivos.

## Código
O arquivo `tarefa4.c` contém:
- Função `memoria_limitada`: soma dois vetores grandes, simulando um cenário limitado pela largura de banda da memória.
- Função `cpu_limitada`: realiza operações matemáticas pesadas em um laço, simulando um cenário limitado por capacidade de processamento.
- O número de threads é definido por prioridade: primeiro pela variável de ambiente `OMP_NUM_THREADS`, depois pelo argumento de linha de comando, e por fim um valor padrão (4).
- O tempo de execução de cada parte é medido e impresso.

## Análise de Desempenho

- **Desempenho melhora:**
  O desempenho aumenta ao adicionar threads até atingir o limite do hardware (núcleos físicos ou largura de banda da memória).
- **Desempenho estabiliza:**
  Quando todos os recursos estão ocupados (CPU ou memória), adicionar mais threads não traz ganhos.
- **Desempenho piora:**
  Se o número de threads excede a capacidade do hardware, pode haver competição por recursos, causando overhead e até piora do tempo de execução.

### Reflexão sobre Multithreading de Hardware
- **Memory-bound:**
  O multithreading de hardware (como Hyper-Threading) pode ajudar, pois enquanto um thread espera dados da memória, outro pode usar a CPU, aproveitando melhor os ciclos ociosos.
- **Compute-bound:**
  O multithreading de hardware pode atrapalhar, pois threads competem diretamente pelos mesmos recursos de execução, aumentando a contenção e reduzindo o desempenho por thread.

## Recomendações
- Para programas memory-bound, usar mais threads pode ser benéfico até saturar a largura de banda da memória.
- Para programas compute-bound, o ideal é usar até o número de núcleos físicos.
- Sempre teste diferentes configurações para encontrar o melhor desempenho para seu hardware.

