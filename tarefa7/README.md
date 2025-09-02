# Tarefa 7 - Processamento Paralelo com Tasks e Lista Encadeada

## Descrição

Este programa demonstra o uso de **OpenMP Tasks** para processamento paralelo de uma lista encadeada. O programa cria uma lista de arquivos fictícios (nomeados com sobrenomes de cientistas famosos) e utiliza tasks para processar cada arquivo de forma paralela e assíncrona.

## Funcionalidades

- **Lista Encadeada**: Estrutura de dados dinâmica contendo nomes de arquivos
- **OpenMP Tasks**: Criação de tarefas paralelas para processamento assíncrono
- **Distribuição Dinâmica**: Tasks são distribuídas automaticamente entre threads disponíveis
- **Sincronização Explícita**: Uso de `barrier`, `taskwait`, `single` e `master`
- **Gerenciamento de Memória**: Liberação adequada da memória alocada

## Conceitos Demonstrados

### 1. **OpenMP Tasks**
```c
#pragma omp task firstprivate(nome_local, task_id)
{
    int thread_executora = omp_get_thread_num();
    processar_arquivo(nome_local, thread_executora, task_id);
}
```

### 2. **Diretiva Single**
```c
#pragma omp single
{
    // Apenas uma thread cria todas as tasks
    // Evita duplicação de trabalho
}
```

### 3. **Diretiva Master**
```c
#pragma omp master
{
    // Executado apenas pela thread master (ID 0)
    // Usado para inicialização e finalização
}
```

### 4. **Barreira de Sincronização**
```c
#pragma omp barrier
{
    // Todas as threads esperam aqui
    // Garante sincronização entre threads
}
```

### 5. **Task Wait**
```c
#pragma omp taskwait
{
    // Espera todas as tasks criadas pela thread atual
    // Sincronização explícita de tasks
}
```

## Estrutura do Programa

### Lista Encadeada
```c
typedef struct No {
    char nome_arquivo[50];
    struct No* proximo;
} No;
```

### Arquivos de Cientistas Famosos
- `Einstein_relativity.txt`
- `Newton_principia.txt`
- `Darwin_evolution.txt`
- `Curie_radioactivity.txt`
- `Tesla_electricity.txt`
- `Hawking_blackholes.txt`
- `Turing_computing.txt`
- `Galileo_astronomy.txt`
- `Mendel_genetics.txt`
- `Pascal_mathematics.txt`

## Fluxo de Execução

1. **Criação da Lista**: Adiciona 10 arquivos fictícios à lista encadeada
2. **Região Paralela**: Inicia região paralela com múltiplas threads
3. **Inicialização Master**: Thread master (ID 0) executa inicialização
4. **Barreira Inicial**: Todas as threads sincronizam antes do processamento
5. **Criação de Tasks**: Uma única thread percorre a lista e cria tasks (single)
6. **Processamento Paralelo**: Tasks são executadas por diferentes threads
7. **Task Wait**: Aguarda explicitamente todas as tasks terminarem
8. **Barreira Final**: Sincronização após conclusão das tasks
9. **Finalização Master**: Thread master executa limpeza final
10. **Limpeza**: Libera memória da lista encadeada

## Comandos OpenMP Utilizados

### 1. **`#pragma omp single`**
- Garante que apenas uma thread executa o bloco
- Usada para criação de tasks (evita duplicação)

### 2. **`#pragma omp master`** 
- Executado apenas pela thread master (ID 0)
- Usado para inicialização e finalização do sistema

### 3. **`#pragma omp barrier`**
- Sincronização explícita entre todas as threads
- Garante que todas chegem ao mesmo ponto antes de continuar

### 4. **`#pragma omp task`**
- Cria tarefas assíncronas para processamento paralelo
- Tasks são distribuídas dinamicamente entre threads

### 5. **`#pragma omp taskwait`**
- Aguarda conclusão de todas as tasks criadas pela thread atual
- Sincronização explícita das tasks

### 1. **Balanceamento Dinâmico**
- Tasks são distribuídas automaticamente
- Threads ociosas pegam novas tasks
- Melhor utilização de recursos

### 2. **Flexibilidade**
- Número variável de tasks
- Criação condicional de tasks
- Aninhamento de tasks possível

### 3. **Desacoplamento**
- Criação e execução são independentes
- Uma thread cria, outras executam
- Escalabilidade natural

## Exemplo de Saída

```
=== PROCESSAMENTO PARALELO DE ARQUIVOS COM TASKS ===
Criando lista de arquivos fictícios...

Número de threads disponíveis: 8
Iniciando processamento paralelo...

Thread master 0 inicializando sistema...
Thread 7 criando tasks para processamento...

==> Task 1 iniciada na Thread 0: Einstein.txt
  -> Thread 0: Analisando conteúdo de Einstein.txt...
==> Task 2 iniciada na Thread 2: Newton.txt
  -> Thread 2: Analisando conteúdo de Newton.txt...

Todas as 10 tasks foram criadas!
Aguardando conclusão de todas as tasks...

  -> Thread 0: Processamento de Einstein.txt concluído!
==> Task 1 finalizada na Thread 0

Thread master 0 finalizando processamento...

=== PROCESSAMENTO CONCLUÍDO ===
Todos os arquivos foram processados com sucesso!
```

## Vantagens das Tasks

### 1. **Balanceamento Dinâmico**
- Tasks são distribuídas automaticamente
- Threads ociosas pegam novas tasks
- Melhor utilização de recursos

### 2. **Flexibilidade**
- Número variável de tasks
- Criação condicional de tasks
- Aninhamento de tasks possível

### 3. **Desacoplamento**
- Criação e execução são independentes
- Uma thread cria, outras executam
- Escalabilidade natural

## Análise de Performance

### Características Observadas

1. **Distribuição Não-Determinística**: 
   - A cada execução, tasks podem ser executadas por threads diferentes
   - Ordem de execução varia devido ao escalonamento dinâmico

2. **Utilização Eficiente**:
   - Múltiplas threads trabalham simultaneamente
   - Balanceamento automático de carga

3. **Overhead Mínimo**:
   - Tasks são criadas rapidamente
   - Sincronização eficiente

## Comparação: Tasks vs Parallel For

| Aspecto | Tasks | Parallel For |
|---------|-------|--------------|
| **Estrutura de Dados** | Qualquer (lista, árvore) | Arrays/loops |
| **Balanceamento** | Dinâmico automático | Estático/manual |
| **Flexibilidade** | Alta | Limitada |
| **Overhead** | Ligeiramente maior | Menor |
| **Casos de Uso** | Trabalho irregular | Trabalho uniforme |

## Conceitos de Programação Paralela

### 1. **Task Parallelism**
- Diferentes threads executam diferentes tarefas
- Contrasta com Data Parallelism (mesmo código, dados diferentes)

### 2. **Work Stealing**
- OpenMP implementa algoritmo de work stealing
- Threads ociosas "roubam" work de threads ocupadas

### 3. **Fork-Join Estendido**
- Tasks estendem o modelo fork-join tradicional
- Maior flexibilidade na criação e execução

**Nota**: Este programa demonstra como OpenMP Tasks fornecem uma abstração poderosa para paralelização de estruturas de dados irregulares, oferecendo balanceamento dinâmico e alta flexibilidade.
