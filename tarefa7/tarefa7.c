#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

// Estrutura do nó da lista encadeada
typedef struct No {
    char nome_arquivo[50];  // Nome do arquivo a ser processado
    struct No* proximo;     // Ponteiro para o próximo nó
} No;

// Função para criar um novo nó
No* criar_no(const char* nome) {
    No* novo_no = (No*)malloc(sizeof(No));  // Aloca memória para novo nó
    if (novo_no != NULL) {
        strcpy(novo_no->nome_arquivo, nome);  // Copia nome do arquivo
        novo_no->proximo = NULL;              // Inicializa ponteiro como NULL
    }
    return novo_no;
}

// Função para adicionar um nó no final da lista
void adicionar_no(No** cabeca, const char* nome) {
    No* novo_no = criar_no(nome);  // Cria novo nó
    if (*cabeca == NULL) {
        *cabeca = novo_no;         // Se lista vazia, novo nó vira cabeça
    } else {
        No* atual = *cabeca;       // Percorre até o final da lista
        while (atual->proximo != NULL) {
            atual = atual->proximo;
        }
        atual->proximo = novo_no;  // Adiciona no final
    }
}

// Função para processar um arquivo (simulação)
void processar_arquivo(const char* nome_arquivo, int thread_id, int task_id) {
    printf("==> Task %d iniciada na Thread %d: %s\n", task_id, thread_id, nome_arquivo);
    
    // Simular algum processamento - sem critical, cada thread imprime independentemente
    printf("  -> Thread %d: Analisando conteúdo de %s...\n", thread_id, nome_arquivo);
    
    // Simular tempo de processamento variável
    for (volatile int i = 0; i < 1000000; i++);  // Loop vazio para simular trabalho
    
    printf("  -> Thread %d: Processamento de %s concluído!\n", thread_id, nome_arquivo);
    printf("==> Task %d finalizada na Thread %d\n\n", task_id, thread_id);
}

// Função para liberar a memória da lista
void liberar_lista(No* cabeca) {
    No* atual = cabeca;
    while (atual != NULL) {
        No* temp = atual;          // Guarda referência para liberar
        atual = atual->proximo;    // Avança para próximo nó
        free(temp);                // Libera nó atual
    }
}

int main() {
    // Criar lista encadeada com nomes de arquivos baseados em cientistas famosos
    No* lista_arquivos = NULL;  // Inicializa lista vazia
    
    printf("=== PROCESSAMENTO PARALELO DE ARQUIVOS COM TASKS ===\n");
    printf("Criando lista de arquivos fictícios...\n\n");
    
    // Adicionar arquivos com nomes de cientistas famosos
    adicionar_no(&lista_arquivos, "Einstein.txt");  
    adicionar_no(&lista_arquivos, "Newton.txt");    
    adicionar_no(&lista_arquivos, "Darwin.txt");    
    adicionar_no(&lista_arquivos, "Curie.txt");     
    adicionar_no(&lista_arquivos, "Tesla.txt");     
    adicionar_no(&lista_arquivos, "Hawking.txt");   
    adicionar_no(&lista_arquivos, "Turing.txt");    
    adicionar_no(&lista_arquivos, "Galileo.txt");   
    adicionar_no(&lista_arquivos, "Mendel.txt");    
    adicionar_no(&lista_arquivos, "Pascal.txt");    

    printf("Número de threads disponíveis: %d\n", omp_get_max_threads());
    printf("Iniciando processamento paralelo...\n\n");
    
    // Região paralela com tasks
    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();  // ID da thread atual
        
        // Master thread faz inicialização
        #pragma omp master
        {
            printf("Thread master %d inicializando sistema...\n", thread_id);
        }
        
        // Barrier para garantir que todas as threads estejam prontas
        #pragma omp barrier
        
        // Apenas uma thread cria as tasks (single)
        #pragma omp single
        {
            printf("Thread %d criando tasks para processamento...\n\n", omp_get_thread_num());
            
            No* atual = lista_arquivos;  // Ponteiro para percorrer lista
            int contador_arquivos = 0;   // Contador de tasks criadas
            
            // Percorrer a lista e criar uma task para cada nó
            while (atual != NULL) {
                contador_arquivos++;
                
                // Capturar o nome do arquivo por valor para evitar race conditions
                char nome_local[50];     // Cópia local do nome
                strcpy(nome_local, atual->nome_arquivo);
                int task_id = contador_arquivos;  // ID da task
                
                // Criar task para processar este arquivo
                #pragma omp task firstprivate(nome_local, task_id)
                {
                    int thread_executora = omp_get_thread_num();  // Thread que executa a task
                    processar_arquivo(nome_local, thread_executora, task_id);
                }
                
                atual = atual->proximo;  // Avança para próximo nó
            }
            
            printf("Todas as %d tasks foram criadas!\n", contador_arquivos);
            printf("Aguardando conclusão de todas as tasks...\n\n");
        }
        
        // Aguardar explicitamente todas as tasks terminarem
        #pragma omp taskwait
        
        // Barrier para sincronizar todas as threads após as tasks - REDUNDANTE
        #pragma omp barrier
        
        // Master thread faz finalização
        #pragma omp master
        {
            printf("Thread master %d finalizando processamento...\n", thread_id);
        }
    }
    
    printf("\n=== PROCESSAMENTO CONCLUÍDO ===\n");
    printf("Todos os arquivos foram processados com sucesso!\n");
    
    // Liberar memória da lista
    liberar_lista(lista_arquivos);  // Limpa toda a lista da memória
    
    return 0;
}