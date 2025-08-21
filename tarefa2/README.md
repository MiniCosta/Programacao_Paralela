# Investigando Paralelismo ao Nível de Instrução (ILP) em C

## Objetivo

Esta atividade tem como objetivo investigar os efeitos do paralelismo ao nível de instrução (ILP) em programas C, analisando como dependências entre iterações afetam o desempenho dos laços sob diferentes níveis de otimização do compilador.

## Teoria

O paralelismo ao nível de instrução (ILP) refere-se à capacidade dos processadores modernos de executar múltiplas instruções simultaneamente, desde que não haja dependências de dados entre elas. O compilador pode explorar ILP reordenando instruções e utilizando recursos internos do processador, mas dependências no código podem limitar esse paralelismo.

Nesta atividade, três laços são implementados para ilustrar diferentes cenários de dependência:

1. **Inicialização de vetor (sem dependências):**
   - Cada elemento do vetor é inicializado de forma independente.
   - O compilador pode paralelizar e otimizar facilmente esse laço.

2. **Soma acumulativa (com dependência):**
   - Os elementos do vetor são somados sequencialmente em uma única variável acumuladora.
   - Existe dependência entre as iterações, pois cada soma depende do resultado anterior, limitando o paralelismo.

3. **Soma com múltiplos acumuladores (quebra de dependência):**
   - A soma é dividida entre múltiplas variáveis acumuladoras.
   - Reduz as dependências entre iterações, permitindo ao compilador explorar mais ILP e paralelismo interno.

## Procedimento

1. **Implemente os três laços em C conforme descrito acima.**
2. **Compile o programa com diferentes níveis de otimização:**
   - `-O0`: Sem otimizações.
   - `-O2`: Otimizações moderadas.
   - `-O3`: Otimizações agressivas, incluindo vetorização e paralelismo.
3. **Meça e compare os tempos de execução de cada laço em cada nível de otimização.**

## Análise

- **Laço 1 (inicialização):** Deve apresentar grande ganho de desempenho com otimizações, pois não há dependências.
- **Laço 2 (soma acumulativa):** O ganho com otimização será limitado devido à dependência sequencial entre as iterações.
- **Laço 3 (soma com múltiplas variáveis):** A quebra de dependência permite ao compilador aplicar ILP, resultando em desempenho superior, especialmente com otimizações mais agressivas.



