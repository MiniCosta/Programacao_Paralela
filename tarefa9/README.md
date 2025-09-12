# Tarefa 9: Regiões críticas nomeadas e Locks explícitos

## Descrição
Este programa demonstra o uso de regiões críticas nomeadas e locks explícitos em OpenMP para realizar inserções concorrentes em múltiplas listas encadeadas, garantindo a integridade dos dados e evitando condições de corrida.

## Mudanças na Implementação (v3.0)

### Versão Mais Limpa
- **Removida demonstração inicial**: Eliminado o teste com 2 listas e elementos predefinidos
- **Foco total na interatividade**: Programa vai direto para entrada do usuário
- **Execução simplificada**: Apenas teste personalizado configurado pelo usuário
- **Interface mais direta**: Entrada → Execução → Resultado

## Características Principais

### 1. Estrutura de Dados
- **Lista Encadeada**: Cada lista possui sua própria estrutura com:
  - Ponteiro para o primeiro nó (`head`)
  - Lock exclusivo (`omp_lock_t`)
  - Contador de elementos (`count`)
  - Identificador único (`id`)

### 2. Locks Explícitos (Regiões Críticas Nomeadas)
- Cada lista possui seu próprio lock (`omp_lock_t`)
- Inserções em listas diferentes podem ocorrer simultaneamente
- Inserções na mesma lista são serializadas automaticamente
- Uso de `omp_set_lock()` e `omp_unset_lock()` para controle explícito

### 3. Funcionalidades

#### Programa Interativo Principal
- Aceita número de listas definido pelo usuário
- Distribui inserções aleatoriamente entre todas as listas
- Escalável para qualquer número de listas
- Interface interativa para configuração personalizada
- Execução direta sem demonstrações preliminares

## Por que Locks Explícitos são Necessários?

### Limitações das Regiões Críticas Tradicionais

#### 1. **Problema da Serialização Global**
```c
// Abordagem INADEQUADA com região crítica global
#pragma omp critical
{
    insert_element(random_list, value);  // TODAS as inserções serializadas!
}
```
**Problema**: Mesmo inserindo em listas diferentes, threads ficam bloqueadas.

#### 2. **Regiões Críticas Nomeadas - Limitação Estática**
```c
// Abordagem LIMITADA com nomes fixos
#pragma omp critical(list1)
{
    insert_element(&list1, value);
}

#pragma omp critical(list2)
{
    insert_element(&list2, value);
}
```
**Problema**: Funciona apenas para número fixo e predefinido de listas.

### Solução com Locks Explícitos

#### 3. **Locks Dinâmicos - Solução Escalável**
```c
// Nossa SOLUÇÃO: lock por lista, dinamicamente alocado
omp_set_lock(&lists[chosen_list].lock);
insert_element(&lists[chosen_list], value);
omp_unset_lock(&lists[chosen_list].lock);
```

### Vantagens dos Locks Explícitos

1. **Granularidade Dinâmica**: Cada lista tem seu lock individual
2. **Escalabilidade**: Funciona com qualquer número N de listas
3. **Paralelismo Máximo**: Threads em listas diferentes não se bloqueiam
4. **Flexibilidade**: Número de recursos protegidos determinado em runtime
5. **Performance**: Contenção mínima entre threads

### Análise Comparativa

| Abordagem | Paralelismo | Escalabilidade | Flexibilidade | Performance |
|-----------|-------------|----------------|---------------|-------------|
| `#pragma omp critical` | ❌ Serialização total | ❌ Não escala | ❌ Inflexível | ❌ Baixa |
| `#pragma omp critical(nome)` | ✅ Parcial | ❌ Limitado | ❌ Estático | 🟡 Média |
| **Locks Explícitos** | ✅ **Máximo** | ✅ **Ilimitado** | ✅ **Dinâmico** | ✅ **Alta** |

### Cenário Prático
Com **4 threads** e **3 listas**:
- **Região crítica global**: Máximo 1 thread ativa (25% uso)
- **Locks explícitos**: Até 3 threads ativas simultaneamente (75% uso)

## Compilação

```bash
gcc -fopenmp -o tarefa9 tarefa9.c -Wall
```

## Execução

```bash
./tarefa9
```

O programa executará:
1. **Entrada interativa** para configuração personalizada

### Exemplo de Uso
```bash
# Execução com entrada automática
echo -e "2\n20\n3" | ./tarefa9

# Execução interativa
./tarefa9
# Digite: 4 listas, 100 inserções, 6 threads
```

## Exemplo de Saída

```
TAREFA 9: Regiões críticas nomeadas e Locks explícitos
======================================================

=== TESTE INTERATIVO ===
Digite o número de listas: 2
Digite o número de inserções: 20
Digite o número de threads: 3

=== PROGRAMA COM 2 LISTAS ===
Número de inserções: 20
Número de threads: 3

Thread 0 inserindo 968 na Lista 2
Thread 1 inserindo 421 na Lista 2    ← Simultâneo!
Thread 0 inserindo 727 na Lista 2
Thread 2 inserindo 649 na Lista 2
Thread 0 inserindo 717 na Lista 1    ← Mudança de lista!
...

Resultados após 20 inserções em 2 listas:
Lista 1 (11 elementos): 432 471 349 384 ...
Lista 2 (9 elementos): 283 102 819 541 ...
Tempo total: 0.0152 segundos
Total de elementos: 20

Programa concluído com sucesso!
```

## Conceitos Demonstrados

### 1. Locks Explícitos vs Regiões Críticas
- **Diferença fundamental**: Locks permitem granularidade dinâmica
- **Escalabilidade**: Funciona com N listas determinado em runtime
- **Performance**: Paralelismo real entre recursos diferentes

### 2. Thread Safety Dinâmica
- **Proteção específica**: Cada lista protegida individualmente
- **Contenção mínima**: Threads só competem pela mesma lista
- **Sincronização eficiente**: Locks apenas quando necessário

### 3. Paralelismo de Granularidade Fina
- **Múltiplos recursos**: Várias listas acessadas simultaneamente
- **Balanceamento**: Distribuição aleatória equilibra carga
- **Escalabilidade**: Performance melhora com mais listas

## Vantagens da Abordagem

1. **Granularidade Dinâmica**: Locks específicos por lista, número variável
2. **Paralelismo Máximo**: Inserções simultâneas em listas diferentes
3. **Flexibilidade Total**: Funciona com qualquer configuração N listas
4. **Segurança Garantida**: Integridade dos dados sem race conditions
5. **Performance Ótima**: Throughput superior a alternativas estáticas
6. **Demonstração Clara**: Visualização do comportamento paralelo

## Estrutura do Código

- `Node`: Estrutura do nó da lista encadeada
- `LinkedList`: Estrutura da lista com lock próprio (`omp_lock_t`)
- `init_list()`: Inicialização de lista com lock explícito
- `insert_element()`: Inserção thread-safe com controle de lock
- `program_n_lists()`: Implementação generalizada para N listas
- `demonstrate_named_critical_sections()`: Demonstração visual dos conceitos
- `main()`: Fluxo simplificado (demonstração + teste interativo)
