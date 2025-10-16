#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>
#include <unistd.h>

// ============================================================================
// ESTRUTURAS DE DADOS
// ============================================================================

// Estrutura do nó da lista encadeada
typedef struct Node {
    int data;
    struct Node* next;
} Node;

// Estrutura para lista simples (usada com regiões críticas nomeadas)
typedef struct {
    Node* head;
    int count;
    int id;
} SimpleList;

// Estrutura para lista com lock explícito (usada com múltiplas listas)
typedef struct {
    Node* head;
    int count;
    int id;
    omp_lock_t lock; // Cada lista tem seu próprio lock independente
} LockedList;

// ============================================================================
// FUNÇÕES AUXILIARES
// ============================================================================

// Função para criar um novo nó
Node* create_node(int data) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (new_node == NULL) {
        fprintf(stderr, "Erro ao alocar memória para novo nó\n");
        exit(1);
    }
    new_node->data = data;
    new_node->next = NULL;
    return new_node;
}

// Inicializar lista simples
void init_simple_list(SimpleList* list, int id) {
    list->head = NULL;
    list->count = 0;
    list->id = id;
}

// Inicializar lista com lock
void init_locked_list(LockedList* list, int id) {
    list->head = NULL;
    list->count = 0;
    list->id = id;
    omp_init_lock(&list->lock); // Inicializa o lock antes do uso
}

// Destruir lista simples
void destroy_simple_list(SimpleList* list) {
    Node* current = list->head;
    while (current != NULL) {
        Node* temp = current;
        current = current->next;
        free(temp);
    }
}

// Destruir lista com lock
void destroy_locked_list(LockedList* list) {
    Node* current = list->head;
    while (current != NULL) {
        Node* temp = current;
        current = current->next;
        free(temp);
    }
    omp_destroy_lock(&list->lock); // Libera recursos do lock
}

// Imprimir lista simples
void print_simple_list(SimpleList* list) {
    printf("Lista %d (%d elementos): ", list->id, list->count);
    Node* current = list->head;
    while (current != NULL) {
        printf("%d ", current->data);
        current = current->next;
    }
    printf("\n");
}

// Imprimir lista com lock
void print_locked_list(LockedList* list) {
    printf("Lista %d (%d elementos): ", list->id, list->count);
    Node* current = list->head;
    while (current != NULL) {
        printf("%d ", current->data);
        current = current->next;
    }
    printf("\n");
}

// ============================================================================
// IMPLEMENTAÇÃO COM DUAS LISTAS USANDO REGIÕES CRÍTICAS NOMEADAS
// ============================================================================

// Variáveis globais para as duas listas (necessário para regiões críticas nomeadas)
SimpleList global_list1, global_list2;

// Função para inserir na lista 1 usando região crítica nomeada
void insert_list1_critical(int data) {
    Node* new_node = create_node(data);
    
    #pragma omp critical(lista1) // Lock específico para lista1 - permite paralelismo com lista2
    {
        new_node->next = global_list1.head;
        global_list1.head = new_node;
        global_list1.count++;
        
        // Simula processamento
        usleep(1000);
    }
}

// Função para inserir na lista 2 usando região crítica nomeada
void insert_list2_critical(int data) {
    Node* new_node = create_node(data);
    
    #pragma omp critical(lista2) // Lock específico para lista2 - independente de lista1
    {
        new_node->next = global_list2.head;
        global_list2.head = new_node;
        global_list2.count++;
        
        // Simula processamento
        usleep(1000);
    }
}

// Programa principal com duas listas usando regiões críticas nomeadas
void program_two_lists_named_critical(int num_insertions, int num_threads) {
    printf("\n=== DUAS LISTAS COM REGIÕES CRÍTICAS NOMEADAS ===\n");
    printf("Inserções: %d | Threads: %d\n\n", num_insertions, num_threads);
    
    // Inicializar as duas listas globais
    init_simple_list(&global_list1, 1);
    init_simple_list(&global_list2, 2);
    
    double start_time = omp_get_wtime(); // Marca tempo inicial
    
    // Região paralela usando TASKS
    #pragma omp parallel num_threads(num_threads)
    {
        #pragma omp single // Apenas uma thread cria as tasks
        {
            // Criar tasks para cada inserção
            for (int i = 0; i < num_insertions; i++) {
                #pragma omp task firstprivate(i) // Cada task tem sua própria cópia de i
                {
                    unsigned int local_seed = time(NULL) + omp_get_thread_num() + i; // Seed único por task
                    
                    // Escolha aleatória entre lista 1 ou 2
                    int choice = rand_r(&local_seed) % 2; // rand_r é thread-safe
                    int value = rand_r(&local_seed) % 1000;
                    
                    if (choice == 0) {
                        insert_list1_critical(value);
                    } else {
                        insert_list2_critical(value);
                    }
                }
            }
        }
        // Todas as tasks terminam aqui automaticamente (taskwait implícito)
    }
    
    double end_time = omp_get_wtime();
    
    printf("\nResultados após %d inserções:\n", num_insertions);
    print_simple_list(&global_list1);
    print_simple_list(&global_list2);
    printf("Tempo total: %.4f segundos\n", end_time - start_time);
    printf("Total de elementos: %d\n", global_list1.count + global_list2.count);
    
    destroy_simple_list(&global_list1);
    destroy_simple_list(&global_list2);
}

// IMPLEMENTAÇÃO GENERALIZADA COM N LISTAS USANDO LOCKS EXPLÍCITOS

// Função para inserir em lista com lock explícito
void insert_locked_list(LockedList* list, int data) {
    Node* new_node = create_node(data);
    
    omp_set_lock(&list->lock); // Adquire lock específico desta lista
    {
        new_node->next = list->head;
        list->head = new_node;
        list->count++;
        
        // Simula processamento
        usleep(1000);
    }
    omp_unset_lock(&list->lock); // Libera lock específico desta lista
}

// Programa generalizado para N listas usando locks explícitos
void program_n_lists_explicit_locks(int num_lists, int num_insertions, int num_threads) {
    printf("\n=== %d LISTAS COM LOCKS EXPLÍCITOS ===\n", num_lists);
    printf("Inserções: %d | Threads: %d\n\n", num_insertions, num_threads);
    
    // Aloca memória para array de listas
    LockedList* lists = (LockedList*)malloc(num_lists * sizeof(LockedList)); // Alocação dinâmica para N listas
    if (lists == NULL) {
        fprintf(stderr, "Erro ao alocar memória para as listas\n");
        exit(1);
    }
    
    // Inicializa todas as listas
    for (int i = 0; i < num_lists; i++) {
        init_locked_list(&lists[i], i + 1); // Cada lista recebe seu próprio lock
    }
    
    double start_time = omp_get_wtime(); // Marca tempo inicial
    
    // Região paralela usando TASKS
    #pragma omp parallel num_threads(num_threads)
    {
        #pragma omp single // Apenas uma thread cria as tasks
        {
            // Criar tasks para cada inserção
            for (int i = 0; i < num_insertions; i++) {
                #pragma omp task firstprivate(i, lists, num_lists) // Cada task tem cópias privadas
                {
                    unsigned int local_seed = time(NULL) + omp_get_thread_num() + i; // Seed único por task
                    
                    // Escolha aleatória entre as N listas
                    int list_choice = rand_r(&local_seed) % num_lists; // Escolha dinâmica entre N listas
                    int value = rand_r(&local_seed) % 1000;
                    
                    insert_locked_list(&lists[list_choice], value); // Acesso direto por índice
                }
            }
        }
        // Todas as tasks terminam aqui automaticamente (taskwait implícito)
    }
    
    double end_time = omp_get_wtime();
    
    printf("\nResultados após %d inserções em %d listas:\n", num_insertions, num_lists);
    int total_elements = 0;
    for (int i = 0; i < num_lists; i++) {
        print_locked_list(&lists[i]);
        total_elements += lists[i].count;
    }
    
    printf("Tempo total: %.4f segundos\n", end_time - start_time);
    printf("Total de elementos: %d\n", total_elements);
    
    // Libera memória das listas
    for (int i = 0; i < num_lists; i++) {
        destroy_locked_list(&lists[i]); // Destrói lock e libera nós de cada lista
    }
    free(lists); // Libera array de listas
}

int main() {
    printf("TAREFA 9: Regiões Críticas Nomeadas vs Locks Explícitos\n");
    printf("========================================================\n");
    
    srand(time(NULL)); // Inicializa gerador de números aleatórios
    
    int num_insertions, num_threads, num_lists;
    
    // Entrada do usuário
    printf("\nDigite o número de inserções: ");
    if (scanf("%d", &num_insertions) != 1 || num_insertions < 1) {
        fprintf(stderr, "Número de inserções inválido\n");
        return 1;
    }
    
    printf("Digite o número de threads: ");
    if (scanf("%d", &num_threads) != 1 || num_threads < 1) {
        fprintf(stderr, "Número de threads inválido\n");
        return 1;
    }
    
    // Demonstração com 2 listas usando regiões críticas nomeadas
    program_two_lists_named_critical(num_insertions, num_threads); // Primeira abordagem: limitada a 2 listas
    
    // Entrada para número de listas
    printf("\nDigite o número de listas para a versão generalizada: ");
    if (scanf("%d", &num_lists) != 1 || num_lists < 1) {
        fprintf(stderr, "Número de listas inválido\n");
        return 1;
    }
    
    // Demonstração com N listas usando locks explícitos
    program_n_lists_explicit_locks(num_lists, num_insertions, num_threads); // Segunda abordagem: escalável para N listas
    
    printf("\nPrograma concluído com sucesso!\n");
    return 0;
}

