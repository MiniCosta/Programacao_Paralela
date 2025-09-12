#include <stdio.h>     
#include <stdlib.h>    
#include <omp.h>      
#include <time.h>      
#include <unistd.h>   

// Estrutura do nó da lista encadeada
typedef struct Node {
    int data;          // Dados armazenados no nó
    struct Node* next; // Ponteiro para o próximo nó
} Node;

// Estrutura para representar uma lista com seu lock
typedef struct {
    Node* head;        // Ponteiro para o primeiro nó da lista
    omp_lock_t lock;   // Lock exclusivo para esta lista específica
    int count;         // Contador de elementos na lista
    int id;           // Identificador único da lista
} LinkedList;

// Função para criar um novo nó
Node* create_node(int data) {
    Node* new_node = (Node*)malloc(sizeof(Node)); // Aloca memória para o novo nó
    if (new_node == NULL) {                        // Verifica se a alocação foi bem-sucedida
        fprintf(stderr, "Erro ao alocar memória para novo nó\n");
        exit(1);                                   // Termina o programa em caso de erro
    }
    new_node->data = data;     // Define o valor do nó
    new_node->next = NULL;     // Inicializa o ponteiro next como NULL
    return new_node;           // Retorna o nó criado
}

// Função para inicializar uma lista
void init_list(LinkedList* list, int id) {
    list->head = NULL;            // Lista começa vazia
    list->count = 0;              // Contador inicia em zero
    list->id = id;                // Define o identificador da lista
    omp_init_lock(&list->lock);   // Inicializa o lock exclusivo da lista
}

// Função para destruir uma lista e liberar memória
void destroy_list(LinkedList* list) {
    Node* current = list->head;    // Começa pelo primeiro nó
    while (current != NULL) {      // Percorre todos os nós
        Node* temp = current;      // Guarda referência do nó atual
        current = current->next;   // Avança para o próximo nó
        free(temp);               // Libera memória do nó atual
    }
    omp_destroy_lock(&list->lock); // Destroi o lock da lista
}

// Função para inserir um elemento na lista (thread-safe)
void insert_element(LinkedList* list, int data) {
    Node* new_node = create_node(data);    // Cria o novo nó a ser inserido
    
    // Região crítica nomeada para esta lista específica
    omp_set_lock(&list->lock);             // Adquire lock exclusivo da lista
    
    printf("Thread %d inserindo %d na Lista %d\n", 
           omp_get_thread_num(), data, list->id); // Mostra qual thread está inserindo
    
    // Inserção no início da lista para simplicidade
    new_node->next = list->head;           // Novo nó aponta para o antigo primeiro
    list->head = new_node;                 // Novo nó torna-se o primeiro
    list->count++;                         // Incrementa contador de elementos
    
    // Simula algum processamento durante a inserção
    usleep(1000);                          // 1ms de delay para visualizar paralelismo
    
    omp_unset_lock(&list->lock);           // Libera o lock da lista
}

// Função para imprimir os elementos de uma lista
void print_list(LinkedList* list) {
    printf("Lista %d (%d elementos): ", list->id, list->count); // Cabeçalho da lista
    Node* current = list->head;        // Começa pelo primeiro nó
    while (current != NULL) {          // Percorre todos os nós
        printf("%d ", current->data);  // Imprime o valor do nó atual
        current = current->next;       // Avança para o próximo nó
    }
    printf("\n");                     // Nova linha ao final
}

// Programa com duas listas
void program_two_lists(int num_insertions, int num_threads) {
    printf("\n=== PROGRAMA COM DUAS LISTAS ===\n");
    printf("Número de inserções: %d\n", num_insertions);
    printf("Número de threads: %d\n\n", num_threads);
    
    LinkedList list1, list2;      // Declara duas listas
    init_list(&list1, 1);         // Inicializa lista 1
    init_list(&list2, 2);         // Inicializa lista 2
    
    double start_time = omp_get_wtime(); // Marca tempo de início
    
    #pragma omp parallel num_threads(num_threads)  // Inicia região paralela
    {
        unsigned int seed = time(NULL) + omp_get_thread_num(); // Seed única por thread
        
        #pragma omp for           // Distribui iterações entre threads
        for (int i = 0; i < num_insertions; i++) {
            // Escolhe aleatoriamente entre lista 1 ou 2
            int choice = rand_r(&seed) % 2;      // 0 ou 1 para escolher lista
            int value = rand_r(&seed) % 1000;    // Valor aleatório entre 0-999
            
            if (choice == 0) {
                insert_element(&list1, value);   // Insere na lista 1
            } else {
                insert_element(&list2, value);   // Insere na lista 2
            }
        }
    }
    
    double end_time = omp_get_wtime();           // Marca tempo de fim
    
    printf("\nResultados após %d inserções:\n", num_insertions);
    print_list(&list1);                         // Mostra conteúdo da lista 1
    print_list(&list2);                         // Mostra conteúdo da lista 2
    printf("Tempo total: %.4f segundos\n", end_time - start_time); // Calcula tempo decorrido
    printf("Total de elementos: %d\n", list1.count + list2.count); // Soma total de elementos
    
    destroy_list(&list1);                       // Libera memória da lista 1
    destroy_list(&list2);                       // Libera memória da lista 2
}

// Programa generalizado para N listas
void program_n_lists(int num_lists, int num_insertions, int num_threads) {
    printf("\n=== PROGRAMA COM %d LISTAS ===\n", num_lists);
    printf("Número de inserções: %d\n", num_insertions);
    printf("Número de threads: %d\n\n", num_threads);
    
    // Aloca memória para array de listas dinamicamente
    LinkedList* lists = (LinkedList*)malloc(num_lists * sizeof(LinkedList));
    if (lists == NULL) {                       // Verifica se alocação foi bem-sucedida
        fprintf(stderr, "Erro ao alocar memória para as listas\n");
        exit(1);                               // Termina programa em caso de erro
    }
    
    // Inicializa todas as listas
    for (int i = 0; i < num_lists; i++) {
        init_list(&lists[i], i + 1);           // Cada lista tem ID sequencial (1, 2, 3...)
    }
    
    double start_time = omp_get_wtime();       // Marca tempo de início
    
    #pragma omp parallel num_threads(num_threads)  // Inicia região paralela
    {
        unsigned int seed = time(NULL) + omp_get_thread_num(); // Seed única por thread
        
        #pragma omp for                        // Distribui iterações entre threads
        for (int i = 0; i < num_insertions; i++) {
            // Escolhe aleatoriamente uma das N listas
            int list_choice = rand_r(&seed) % num_lists;    // Índice da lista escolhida
            int value = rand_r(&seed) % 1000;               // Valor aleatório entre 0-999
            
            insert_element(&lists[list_choice], value);     // Insere na lista escolhida
        }
    }
    
    double end_time = omp_get_wtime();         // Marca tempo de fim
    
    printf("\nResultados após %d inserções em %d listas:\n", num_insertions, num_lists);
    int total_elements = 0;                    // Contador total de elementos
    for (int i = 0; i < num_lists; i++) {
        print_list(&lists[i]);                 // Mostra conteúdo de cada lista
        total_elements += lists[i].count;      // Soma elementos de todas as listas
    }
    
    printf("Tempo total: %.4f segundos\n", end_time - start_time); // Calcula tempo decorrido
    printf("Total de elementos: %d\n", total_elements);            // Mostra total de elementos
    
    // Libera memória das listas
    for (int i = 0; i < num_lists; i++) {
        destroy_list(&lists[i]);               // Destroi cada lista individualmente
    }
    free(lists);                              // Libera array de listas
}

int main() {
    printf("TAREFA 9: Regiões críticas nomeadas e Locks explícitos\n");
    
    srand(time(NULL));                         // Inicializa gerador de números aleatórios
    
    // Teste interativo para que o usuário defina o número de listas
    printf("\n=== TESTE INTERATIVO ===\n");
    int num_lists, num_insertions, num_threads; // Variáveis para entrada do usuário
    
    printf("Digite o número de listas: ");
    if (scanf("%d", &num_lists) != 1 || num_lists < 1) { // Lê e valida número de listas
        fprintf(stderr, "Número de listas inválido\n");
        return 1;                              // Retorna erro se entrada inválida
    }
    
    printf("Digite o número de inserções: ");
    if (scanf("%d", &num_insertions) != 1 || num_insertions < 1) { // Lê e valida número de inserções
        fprintf(stderr, "Número de inserções inválido\n");
        return 1;                              // Retorna erro se entrada inválida
    }
    
    printf("Digite o número de threads: ");
    if (scanf("%d", &num_threads) != 1 || num_threads < 1) { // Lê e valida número de threads
        fprintf(stderr, "Número de threads inválido\n");
        return 1;                              // Retorna erro se entrada inválida
    }
    
    program_n_lists(num_lists, num_insertions, num_threads); // Executa programa principal
    
    printf("\nPrograma concluído com sucesso!\n");
    return 0;                                  // Retorna sucesso
}

/*
INSTRUÇÕES DE COMPILAÇÃO E EXECUÇÃO

COMPILAÇÃO:
-----------
Para compilar o programa, use o seguinte comando:

    gcc -fopenmp -o tarefa9 tarefa9.c -Wall

EXECUÇÃO:
---------
Para executar o programa:

    ./tarefa9

O programa irá:
1. Solicitar entrada do usuário para teste personalizado

EXEMPLO DE EXECUÇÃO COM ENTRADA AUTOMÁTICA:
------------------------------------------
Para fornecer entrada automaticamente:

    echo -e "3\n75\n4" | ./tarefa9

Isso executará com 3 listas, 75 inserções e 4 threads.
*/
