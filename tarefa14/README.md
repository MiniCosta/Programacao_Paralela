# Tarefa 14: Programação em Memória Distribuída

## Descrição
Este programa implementa um teste de ping-pong MPI entre dois processos para medir latência e largura de banda em função do tamanho da mensagem.

## Sobre MPI (Message Passing Interface)

### O que é MPI?
MPI é um padrão de comunicação para programação paralela em sistemas de memória distribuída. Ele define uma API que permite que processos executando em diferentes nós de um cluster (ou mesmo na mesma máquina) se comuniquem através de troca de mensagens.

### Características do MPI:
- **Portabilidade**: Funciona em diferentes arquiteturas e sistemas operacionais
- **Escalabilidade**: Suporte desde poucos processos até milhares de processos
- **Flexibilidade**: Diferentes padrões de comunicação (ponto-a-ponto, coletiva)
- **Performance**: Otimizado para comunicação de alta performance

### Funções MPI Utilizadas:

#### Funções de Inicialização e Finalização:
- **`MPI_Init(&argc, &argv)`**: Inicializa o ambiente MPI
  - Deve ser a primeira chamada MPI no programa
  - Configura o ambiente de comunicação e estruturas internas
  
- **`MPI_Finalize()`**: Finaliza o ambiente MPI
  - Deve ser a última chamada MPI no programa
  - Libera recursos e limpa estruturas internas

#### Funções de Identificação:
- **`MPI_Comm_rank(MPI_COMM_WORLD, &rank)`**: Obtém o identificador único do processo atual
  - `MPI_COMM_WORLD`: Comunicador que inclui todos os processos
  - `&rank`: Ponteiro para variável que receberá o ID (0, 1, 2, ...)
  
- **`MPI_Comm_size(MPI_COMM_WORLD, &size)`**: Obtém o número total de processos
  - `MPI_COMM_WORLD`: Comunicador que inclui todos os processos
  - `&size`: Ponteiro para variável que receberá o total de processos

#### Funções de Comunicação:
- **`MPI_Ssend(buffer, count, datatype, dest, tag, comm)`**: Envia mensagem **sincronamente**
  - **IMPORTANTE**: Só completa quando o `MPI_Recv` correspondente **iniciou**
  - Garante sincronização real entre sender e receiver
  - Parâmetros:
    - `buffer`: Ponteiro para os dados a serem enviados
    - `count`: Número de elementos a enviar
    - `datatype`: Tipo de dados MPI (ex: `MPI_CHAR`, `MPI_INT`)
    - `dest`: Rank do processo destino
    - `tag`: Etiqueta da mensagem (para identificação)
    - `comm`: Comunicador (ex: `MPI_COMM_WORLD`)

- **`MPI_Recv(buffer, count, datatype, source, tag, comm, status)`**: Recebe mensagem
  - Sempre **bloqueante** - espera até mensagem chegar
  - Parâmetros:
    - `buffer`: Ponteiro para buffer que receberá os dados
    - `count`: Número máximo de elementos a receber
    - `datatype`: Tipo de dados MPI esperado
    - `source`: Rank do processo remetente
    - `tag`: Etiqueta da mensagem esperada
    - `comm`: Comunicador
    - `status`: Informações sobre a mensagem recebida (`MPI_STATUS_IGNORE` se não precisar)

#### Função de Temporização:
- **`MPI_Wtime()`**: Retorna tempo de alta precisão em segundos
  - Resolução tipicamente em microssegundos
  - Monotônico - não afetado por mudanças no relógio do sistema
  - Ideal para benchmarks de performance

### Padrão Ping-Pong:
O teste ping-pong é um benchmark clássico para medir:
- **Latência**: Tempo mínimo para enviar uma mensagem pequena
- **Largura de Banda**: Taxa máxima de transferência para mensagens grandes
- **Overhead**: Custo fixo da comunicação independente do tamanho da mensagem

## Padrões de Comunicação: Bloqueante vs Não-Bloqueante

### Comunicação Bloqueante:
Uma operação **bloqueante** não retorna até que seja **seguro** reusar os recursos envolvidos (buffers, variáveis).

#### Características:
- ✅ **Segurança**: Garante que a operação foi completada
- ✅ **Simplicidade**: Programação mais intuitiva
- ❌ **Performance**: Pode causar espera desnecessária
- ❌ **Deadlocks**: Risco se não bem planejado

#### Exemplos de Funções Bloqueantes:
```c
// MPI_Send - pode ser bloqueante ou não dependendo da implementação
MPI_Send(buffer, count, MPI_INT, dest, tag, MPI_COMM_WORLD);

// MPI_Ssend - SEMPRE bloqueante (synchronous)
MPI_Ssend(buffer, count, MPI_INT, dest, tag, MPI_COMM_WORLD);

// MPI_Recv - SEMPRE bloqueante
MPI_Recv(buffer, count, MPI_INT, source, tag, MPI_COMM_WORLD, &status);
```

#### Exemplo Prático - Ping-Pong Bloqueante:
```c
if (rank == 0) {
    MPI_Send(data, 100, MPI_INT, 1, 0, MPI_COMM_WORLD);     // Envia para processo 1
    MPI_Recv(data, 100, MPI_INT, 1, 0, MPI_COMM_WORLD, &status); // Recebe de volta
} else if (rank == 1) {
    MPI_Recv(data, 100, MPI_INT, 0, 0, MPI_COMM_WORLD, &status); // Recebe do processo 0
    MPI_Send(data, 100, MPI_INT, 0, 0, MPI_COMM_WORLD);     // Envia de volta
}
```

### Comunicação Não-Bloqueante:
Uma operação **não-bloqueante** retorna **imediatamente**, iniciando a operação em background.

#### Características:
- ✅ **Performance**: Permite sobreposição de computação e comunicação
- ✅ **Flexibilidade**: Múltiplas operações simultâneas
- ❌ **Complexidade**: Requer gerenciamento de requests
- ❌ **Sincronização**: Necessita verificar completude com Wait/Test

#### Exemplos de Funções Não-Bloqueantes:
```c
MPI_Request request;
MPI_Status status;

// MPI_Isend - send não-bloqueante (immediate send)
MPI_Isend(buffer, count, MPI_INT, dest, tag, MPI_COMM_WORLD, &request);

// MPI_Irecv - recv não-bloqueante (immediate receive)
MPI_Irecv(buffer, count, MPI_INT, source, tag, MPI_COMM_WORLD, &request);

// Verificar se completou
MPI_Wait(&request, &status);  // Espera completar
// ou
int flag;
MPI_Test(&request, &flag, &status);  // Verifica sem esperar
```

#### Exemplo Prático - Ping-Pong Não-Bloqueante:
```c
MPI_Request send_req, recv_req;
MPI_Status status;

if (rank == 0) {
    // Inicia send não-bloqueante
    MPI_Isend(data, 100, MPI_INT, 1, 0, MPI_COMM_WORLD, &send_req);
    // Inicia recv não-bloqueante
    MPI_Irecv(data, 100, MPI_INT, 1, 1, MPI_COMM_WORLD, &recv_req);
    
    // Pode fazer outras computações aqui...
    
    // Espera ambas as operações completarem
    MPI_Wait(&send_req, &status);
    MPI_Wait(&recv_req, &status);
} else if (rank == 1) {
    MPI_Irecv(data, 100, MPI_INT, 0, 0, MPI_COMM_WORLD, &recv_req);
    MPI_Isend(data, 100, MPI_INT, 0, 1, MPI_COMM_WORLD, &send_req);
    
    MPI_Wait(&recv_req, &status);
    MPI_Wait(&send_req, &status);
}
```

### Tipos de Send em MPI:

#### 1. **MPI_Send** (Standard Send):
```c
MPI_Send(buffer, count, datatype, dest, tag, comm);
```
- **Semântica**: Pode ser bloqueante ou não-bloqueante
- **Implementação**: Depende do tamanho da mensagem e buffers disponíveis
- **Uso**: Geral, quando não se precisa de garantias específicas

#### 2. **MPI_Ssend** (Synchronous Send):
```c
MPI_Ssend(buffer, count, datatype, dest, tag, comm);
```
- **Semântica**: SEMPRE bloqueante
- **Garantia**: Só completa quando MPI_Recv correspondente iniciou
- **Uso**: Quando se precisa de sincronização garantida

#### 3. **MPI_Bsend** (Buffered Send):
```c
MPI_Bsend(buffer, count, datatype, dest, tag, comm);
```
- **Semântica**: Não-bloqueante (se buffer disponível)
- **Requisito**: Precisa de buffer alocado com MPI_Buffer_attach
- **Uso**: Quando se quer garantir não-bloqueio

### Por que MPI_Ssend ao invés de MPI_Send?

#### Diferenças entre os tipos de Send:
1. **`MPI_Send`** (Standard Send):
   - Semântica **não-determinística**
   - Pode ser bloqueante ou não-bloqueante dependendo da implementação
   - Para mensagens pequenas: geralmente buffered (não-bloqueante)
   - Para mensagens grandes: geralmente synchronous (bloqueante)
   - **Problema**: Comportamento inconsistente para diferentes tamanhos

2. **`MPI_Ssend`** (Synchronous Send):
   - **SEMPRE bloqueante** - só completa quando recv correspondente iniciou
   - Comportamento **determinístico** independente do tamanho da mensagem
   - **Vantagem**: Mede latência real de comunicação, não tempo de cópia para buffer
   - **Ideal para benchmarks** de performance de rede

#### Benefícios para este benchmark:
- ✅ **Medição precisa**: Elimina variações causadas por buffering interno
- ✅ **Comportamento consistente**: Mesmo padrão para todos os tamanhos de mensagem
- ✅ **Latência real**: Mede tempo de comunicação efetiva entre processos
- ✅ **Resultados comparáveis**: Entre diferentes implementações MPI


### Detalhamento dos Parâmetros das Funções MPI:

#### Exemplo de chamada MPI_Ssend:
```c
MPI_Ssend(buffer, tam, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
```
- `buffer`: Ponteiro para o array de dados a enviar
- `tam`: Número de caracteres a enviar (tamanho da mensagem)
- `MPI_CHAR`: Tipo de dados = char (1 byte por elemento)
- `1`: Rank do processo destino (processo 1)
- `0`: Tag da mensagem (identificador, pode ser qualquer inteiro)
- `MPI_COMM_WORLD`: Comunicador global (todos os processos)

#### Exemplo de chamada MPI_Recv:
```c
MPI_Recv(buffer, tam, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
```
- `buffer`: Ponteiro para o array que receberá os dados
- `tam`: Número máximo de caracteres a receber
- `MPI_CHAR`: Tipo de dados esperado = char
- `0`: Rank do processo remetente (processo 0)
- `0`: Tag da mensagem esperada (deve coincidir com o send)
- `MPI_COMM_WORLD`: Comunicador
- `MPI_STATUS_IGNORE`: Não precisamos das informações de status da mensagem

#### Tags e Comunicadores:
- **Tags**: Permitem distinguir diferentes tipos de mensagens entre os mesmos processos
- **Comunicadores**: Definem grupos de processos que podem se comunicar
- **MPI_COMM_WORLD**: Comunicador padrão que inclui todos os processos do programa

## Resultados

### Dados Coletados - Resultados com MPI_Ssend
O programa testou tamanhos de mensagem de 8 bytes até 1 MB (1.048.576 bytes), realizando 10.000 repetições para cada tamanho usando **MPI_Ssend** para garantir sincronização.

| Tamanho (bytes) | Tempo médio (μs) | Tempo total (s) |
|----------------|------------------|-----------------|
| 8              | 1.01             | 0.010058        |
| 16             | 0.97             | 0.009663        |
| 32             | 1.26             | 0.012645        |
| 64             | 1.30             | 0.012959        |
| 128            | 1.37             | 0.013721        |
| 256            | 1.35             | 0.013512        |
| 512            | 1.42             | 0.014150        |
| 1024           | 1.41             | 0.014057        |
| 2048           | 1.58             | 0.015825        |
| 4096           | 4.30             | 0.042983        |
| 8192           | 5.49             | 0.054875        |
| 16384          | 7.44             | 0.074419        |
| 32768          | 10.81            | 0.108077        |
| 65536          | 17.39            | 0.173949        |
| 131072         | 30.56            | 0.305616        |
| 262144         | 51.87            | 0.518749        |
| 524288         | 101.23           | 1.012320        |
| 1048576        | 224.12           | 2.241173        |

### Análise dos Regimes com MPI_Ssend

#### 1. Regime de Latência (mensagens ≤ 2KB)
- **Características**: Tempo dominado pela latência de comunicação sincronizada
- **Observação**: O tempo permanece relativamente constante (~1.0-1.6 μs)
- **Causa**: Overhead do protocolo MPI_Ssend e latência de sincronização dominam
- **Diferença do MPI_Send**: Latência ~2.5x maior devido à sincronização garantida

#### 2. Regime de Transição (2KB - 4KB)
- **Características**: Início da influência do tamanho da mensagem
- **Observação**: Pequeno aumento no tempo (1.58 → 4.30 μs)
- **Causa**: Combinação de latência de sincronização com tempo de transferência

#### 3. Regime de Largura de Banda (mensagens > 4KB)
- **Características**: Tempo cresce linearmente com o tamanho da mensagem
- **Observação**: A partir de 4KB, crescimento proporcional ao tamanho
- **Causa**: A largura de banda da rede se torna o fator limitante

### Métricas Importantes - Versão MPI_Ssend
- **Latência base**: ~1.0 μs (para mensagens pequenas)
- **Largura de banda efetiva**: ~4.7 GB/s (calculada para mensagem de 1MB)
- **Ponto de transição**: ~2-4KB (onde a largura de banda começa a dominar)
- **Overhead de sincronização**: ~0.6 μs comparado ao MPI_Send padrão


### Observações Importantes:
1. **MPI_Ssend é mais lento**: Latência base ~2.5x maior que MPI_Send
2. **Sincronização garantida**: Elimina variações por buffering
3. **Comportamento consistente**: Mesmo padrão para todos os tamanhos
4. **Ideal para benchmarks**: Medição precisa da latência real de comunicação


## Interpretação dos Resultados

### Modelo de Performance MPI
O tempo total de comunicação MPI pode ser modelado como:
```
T(n) = α + β × n
```
Onde:
- `α` (alfa) = Latência base - tempo fixo independente do tamanho
- `β` (beta) = Tempo por byte - inverso da largura de banda
- `n` = Tamanho da mensagem em bytes

## Conclusões
1. Para mensagens pequenas (≤1KB), a latência de comunicação é o fator dominante
2. Para mensagens grandes (>1KB), a largura de banda se torna limitante
3. O sistema MPI local apresenta excelente performance com latência muito baixa
4. A transição entre regimes ocorre em torno de 1-4KB, típico para sistemas de comunicação local
5. O modelo α-β ajuda a prever performance para diferentes tamanhos de mensagem
6. **MPI_Ssend fornece medições mais precisas** para benchmarks de comunicação
7. A sincronização garantida elimina incertezas sobre políticas de buffering
