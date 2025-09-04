# Tarefa 6 - Estimativa Estocástica de π com OpenMP

## Descrição
Este programa implementa o método de Monte Carlo para estimar o valor de π, demonstrando problemas de condições de corrida e diferentes cláusulas OpenMP.

## Conceito do Método de Monte Carlo
O método de Monte Carlo estima π gerando pontos aleatórios em um quadrado de lado 2 (coordenadas de -1 a 1) e contando quantos caem dentro de um círculo de raio 1:

- **Área do quadrado**: 4 (2×2)
- **Área do círculo**: π (π×1²)
- **Relação**: pontos_dentro_círculo / total_pontos ≈ π/4
- **Estimativa de π**: 4 × (pontos_dentro_círculo / total_pontos)

## Implementações Desenvolvidas

### 1. Versão Sequencial (Referência)
```c
double estimar_pi_sequencial(long num_pontos);
```
- Implementação básica sem paralelização
- Serve como referência para comparação de resultados e performance

### 2. Versão com Condição de Corrida (INCORRETA)
```c
#pragma omp parallel for
for (long i = 0; i < num_pontos; i++) {
    // ... cálculos ...
    if (x*x + y*y <= 1.0) {
        pontos_dentro++; // PROBLEMA: Race condition!
    }
}
```

**Problema**: Múltiplas threads incrementam `pontos_dentro` simultaneamente sem sincronização.

**Resultados observados** (execução atual):
- π estimado: 0.909280 (erro: 71.057%)
- π estimado: 0.823200 (erro: 73.797%)  
- π estimado: 0.906520 (erro: 71.145%)

**Variabilidade em múltiplas execuções**:
- Faixa de valores: 0.430480 a 1.652680
- Faixa de erros: 43.200% a 86.297%
- Resultados completamente inconsistentes

**Explicação**: O incremento `pontos_dentro++` não é atômico. Múltiplas threads podem:
1. Ler o mesmo valor
2. Incrementar localmente
3. Escrever de volta, perdendo incrementos

### 3. Correção com `#pragma omp critical`
```c
if (x*x + y*y <= 1.0) {
    #pragma omp critical
    pontos_dentro++;
}
```

**Solução**: Apenas uma thread por vez pode executar o bloco critical.
**Desvantagem**: Serializa o acesso, reduzindo performance (tempo: 0.1038s vs 0.0173s sequencial).

### 4. Versão Otimizada com `reduction`
```c
#pragma omp parallel for reduction(+:pontos_dentro)
```

**Vantagem**: OpenMP gerencia automaticamente a redução:
- Cada thread mantém uma cópia local de `pontos_dentro`
- No final, todas as cópias são somadas eficientemente
- Melhor performance que critical (tempo: 0.0033s)

## Demonstração das Cláusulas OpenMP

### PRIVATE
```c
#pragma omp parallel private(thread_id)
```
- Cada thread tem sua própria cópia da variável
- Valor inicial é **indefinido**
- Modificações não afetam outras threads
- Valor **não é preservado** após a região paralela

### FIRSTPRIVATE  
```c
#pragma omp parallel firstprivate(valor_inicial)
```
- Como `private`, mas **inicializada** com o valor original
- Cada thread recebe uma cópia com o valor da thread mestre
- Útil quando threads precisam do valor inicial para cálculos

### LASTPRIVATE
```c
#pragma omp parallel for lastprivate(ultimo_indice)
```
- Como `private`, mas **preserva** o valor da última iteração
- A thread que executa a última iteração define o valor final
- Valor é copiado de volta para a variável original

### SHARED
```c
#pragma omp parallel shared(contador_global)
```
- Variável é **compartilhada** entre todas as threads
- Todas acessam a mesma localização de memória
- **Requer sincronização** para evitar condições de corrida

## Resultados da Execução

### Demonstração das Cláusulas
Sistema com 8 threads disponíveis executando 1.000.000 de pontos:

- **SEQUENCIAL**: π=3.142568, erro=0.031%, tempo=0.0144s
- **RACE CONDITION**: Resultados inconsistentes
  - Execução 1: π=0.909280 (71.057% erro)
  - Execução 2: π=0.823200 (73.797% erro) 
  - Execução 3: π=0.906520 (71.145% erro)
- **CRITICAL**: π=3.141136, erro=0.015%, tempo=0.1326s
- **REDUCTION**: π=3.141120, erro=0.015%, tempo=0.0215s
- **CLÁUSULAS**: π=3.139200, erro=0.076%, tempo=0.0056s (10.000 pontos)
- **LASTPRIVATE**: π=3.139200, erro=0.076%, tempo=0.0041s (10.000 pontos)

## Performance Comparada

| Versão | Tempo (s) | π Estimado | Erro (%) | Observações |
|--------|-----------|------------|-----------|-------------|
| Sequencial | 0.0144 | 3.142568 | 0.031% | Referência |
| Race Condition | 0.0168-0.0281 | 0.430-1.653 | 43-86% | **Altamente inconsistente** |
| Critical | 0.1326 | 3.141136 | 0.015% | Correto, mas 9x mais lento |
| Reduction | 0.0215 | 3.141120 | 0.015% | **Melhor solução** |
| Cláusulas | 0.0056 | 3.139200 | 0.076% | Demonstração educativa |
| Lastprivate | 0.0041 | 3.139200 | 0.076% | Menor dataset para demo |

## Análise dos Resultados Obtidos

### Observações Importantes

1. **Overhead da Paralelização**: A versão com reduction (0.0215s) é ligeiramente mais lenta que a sequencial (0.0144s) para este problema, indicando que o overhead de criação e sincronização de threads supera o ganho computacional para este tamanho de problema.

2. **Critical Section - Gargalo Severo**: A versão com critical section (0.1326s) é **9 vezes mais lenta** que a sequencial, demonstrando o impacto devastador da serialização forçada.

3. **Variabilidade Extrema na Race Condition**: Os valores variam de 0.430 a 1.653, com erros de 43% a 86%, mostrando que a condição de corrida não apenas produz resultados incorretos, mas completamente imprevisíveis.

4. **Consistência das Soluções Corretas**: Tanto critical quanto reduction produzem sempre o mesmo resultado (π=3.141136 e π=3.141120 respectivamente), demonstrando que as correções funcionam.

## Conclusões

1. **Condições de corrida** são um problema sério na paralelização
2. **Critical sections** resolvem o problema, mas com custo de performance
3. **Reduction** é a solução mais eficiente para operações associativas
4. **Cláusulas OpenMP** oferecem controle fino sobre escopo de variáveis:
   - Use `private` para variáveis temporárias
   - Use `firstprivate` quando precisar do valor inicial
   - Use `lastprivate` para preservar resultado final
   - Use `shared` para dados que devem ser compartilhados
   - Use `reduction` para operações de acúmulo

## Compilação e Execução

```bash
gcc -fopenmp -lm -o tarefa6 tarefa6.c
./tarefa6
```

**Observação**: Execute múltiplas vezes para observar a inconsistência da versão com race condition.
