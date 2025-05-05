#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include "../include/server.h"

void init_cache(Cache *cache, int capacity)
{
    cache->items = malloc(capacity * sizeof(DocumentMetadata));
    cache->capacity = capacity;
    cache->size = 0;
    cache->access_times = malloc(capacity * sizeof(int));
    cache->current_time = 0;
}

void free_cache(Cache *cache)
{
    free(cache->items);
    free(cache->access_times);
}

DocumentMetadata *get_from_cache(Cache *cache, int key)
{
    for (int i = 0; i < cache->size; i++)
    {
        if (cache->items[i].key == key)
        {
            cache->access_times[i] = ++cache->current_time;
            return &cache->items[i];
        }
    }
    return NULL;
}

void add_to_cache(Cache *cache, DocumentMetadata doc)
{
    // Verifica se já está na cache
    for (int i = 0; i < cache->size; i++)
    {
        if (cache->items[i].key == doc.key)
        {
            cache->items[i] = doc;
            cache->access_times[i] = ++cache->current_time;
            return;
        }
    }

    // Adiciona novo item se houver espaço
    if (cache->size < cache->capacity)
    {
        cache->items[cache->size] = doc;
        cache->access_times[cache->size] = ++cache->current_time;
        cache->size++;
    }
    else
    {
        // Substitui o item menos recentemente usado LRU
        int lru_index = 0;
        for (int i = 1; i < cache->size; i++)
        {
            if (cache->access_times[i] < cache->access_times[lru_index])
            {
                lru_index = i;
            }
        }
        cache->items[lru_index] = doc;
        cache->access_times[lru_index] = ++cache->current_time;
    }
}

void save_metadata(ServerState *state)
{
    int fd = open(METADATA_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
    {
        perror("Erro ao abrir o arquivo de metadados");
        return;
    }

    write(fd, &state->count, sizeof(int));
    write(fd, &state->next_key, sizeof(int));

    for (int i = 0; i < state->count; i++)
    {
        if (write(fd, &state->documents[i], sizeof(DocumentMetadata)) == -1)
        {
            perror("Erro ao escrever no arquivo de metadados");
            return;
        }
    }
    close(fd);
}

void load_metadata(ServerState *state)
{
    int fd = open(METADATA_FILE, O_RDONLY);
    if (fd == -1)
    {
        state->documents = NULL;
        state->count = 0;
        state->next_key = 1;
        return;
    }

    char test_byte;
    if (read(fd, &test_byte, sizeof(char)) == -1)
    {
        close(fd);
        state->documents = NULL;
        state->count = 0;
        state->next_key = 1;
        return;
    }

    lseek(fd, 0, SEEK_SET);

    if (read(fd, &state->count, sizeof(int)) == -1)
    {
        close(fd);
        perror("Erro ao ler o número de documentos");
        return;
    }

    state->documents = malloc(state->count * sizeof(DocumentMetadata));
    if (state->documents == NULL)
    {
        perror("Erro ao alocar memória para documentos");
        close(fd);
        return;
    }

    for (int i = 0; i < state->count; i++)
    {
        if (read(fd, &state->documents[i], sizeof(DocumentMetadata)) == -1)
        {
            free(state->documents);
            close(fd);
            perror("Erro ao ler os documentos");
            return;
        }
    }
    close(fd);
    printf("Metadados carregados com sucesso. Total de documentos: %d\n", state->count);
}

// int count_lines_with_keyword(int key, const char *keyword)
//{
// }
//
// int *search_documents_with_keyword(const char *keyword, int *result_count)
//{
// }

void free_resources(ServerState *state)
{
    free(state->documents);
    free_cache(&state->cache);
}

DocumentMetadata *get_document(ServerState *state, int key)
{
    DocumentMetadata *doc = get_from_cache(&state->cache, key);
    if (doc)
        return doc;

    for (int i = 0; i < state->count; i++)
    {
        if (state->documents[i].key == key)
        {
            add_to_cache(&state->cache, state->documents[i]);
            return &state->documents[i];
        }
    }
    return NULL;
}

int remove_document(ServerState *state, int key) // ??
{
    for (int i = 0; i < state->count; i++)
    {
        if (state->documents[i].key == key)
        {
            // Remove o documento da lista
            for (int j = i; j < state->count - 1; j++)
            {
                state->documents[j] = state->documents[j + 1];
            }
            state->count--;
            save_metadata(state);
            return 0;
        }
    }
    return -1;
}
void init_server(ServerState *state, const char *document_folder, int cache_size)
{
    strncpy(state->document_folder, document_folder, sizeof(state->document_folder) - 1);
    state->next_key = 1;
    state->count = 0;
    state->documents = NULL;

    init_cache(&state->cache, cache_size);
    load_metadata(state);
}

int add_document(ServerState *state, DocumentMetadata doc)
{
    // Veerificar se o documento já existe
    for (int i = 0; i < state->count; i++)
    {
        if (strcmp(state->documents[i].path, doc.path) == 0)
        {
            printf("Documento já existe.\n");
            return -1;
        }
    }

    // Verificar se o documento existe

    // Alocar espaço se necessário
    DocumentMetadata *new_documents = realloc(state->documents, (state->count + 1) * sizeof(DocumentMetadata));
    if (new_documents == NULL)
    {
        perror("Erro ao alocar memória para documentos");
        return -1;
    }

    state->documents = new_documents;

    // Adicionar o novo documento
    doc.key = state->next_key++;
    state->documents[state->count] = doc;
    state->count++;
    add_to_cache(&state->cache, doc);
    printf("Documento adicionado com sucesso. Key: %d\n", doc.key);
    save_metadata(state);
    return 0;
}

void handle_command(ServerState *state, const char *command)
{
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s document_folder cache_size\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Verificar tamanho da cache
    int cache_size = atoi(argv[2]);
    if (cache_size <= 0) {
        fprintf(stderr, "Erro: cache_size deve ser positivo\n");
        return EXIT_FAILURE;
    }

    // Alocar e inicializar estado do servidor
    ServerState *state = malloc(sizeof(ServerState));
    if (!state) {
        perror("Erro ao alocar memória para o servidor");
        return EXIT_FAILURE;
    }
    init_server(state, argv[1], cache_size);

    // Remover FIFOs existentes (ignorar erros se não existirem)
    unlink(SERVER_PIPE);
    unlink(CLIENT_PIPE);

    // Criar novos FIFOs
    if (mkfifo(SERVER_PIPE, 0666) == -1) {
        perror("Erro ao criar pipe do servidor");
        free(state);
        return EXIT_FAILURE;
    }

    if (mkfifo(CLIENT_PIPE, 0666) == -1) {
        perror("Erro ao criar pipe de respostas");
        unlink(SERVER_PIPE);
        free(state);
        return EXIT_FAILURE;
    }

    printf("Servidor iniciado. Documentos em: %s | Cache: %d itens\n",
           state->document_folder, state->cache.capacity);

    // Abrir pipe do servidor
    int pipe_fd = open(SERVER_PIPE, O_RDONLY);
    if (pipe_fd == -1) {
        perror("Erro ao abrir pipe do servidor");
        unlink(SERVER_PIPE);
        unlink(CLIENT_PIPE);
        free(state);
        return EXIT_FAILURE;
    }

    // Loop principal do servidor
    char buffer[BUFFER_SIZE];
    while (1) {
        ssize_t bytes_read = read(pipe_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            if (strcmp(buffer, "-f") == 0) {
                break;  // Comando de desligamento
            }
            handle_command(state, buffer);
        } else if (bytes_read == -1) {
            perror("Erro ao ler do pipe");
            break;
        }
    }

    // Limpeza final
    close(pipe_fd);
    unlink(SERVER_PIPE);
    unlink(CLIENT_PIPE);
    save_metadata(state);
    free_resources(state);
    free(state);

    printf("Servidor encerrado.\n");
    return EXIT_SUCCESS;
}