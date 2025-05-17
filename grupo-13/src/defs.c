#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../include/server.h"
#include "../include/defs.h"

int global_key = 0;

int find_lowest_available_key()
{
    int fd_meta = open(INDEX_FILE, O_RDONLY);
    if (fd_meta == -1)
        return 1;

    int max_key = global_key > 0 ? global_key : 1;
    int *used_keys = calloc(max_key + 2, sizeof(int));
    if (!used_keys)
    {
        close(fd_meta);
        return max_key + 1;
    }

    ArchiveMetadata entry;
    lseek(fd_meta, 0, SEEK_SET);
    while (read(fd_meta, &entry, sizeof(ArchiveMetadata)) > 0)
    {
        if (entry.key > 0 && entry.key <= max_key + 1)
        {
            used_keys[entry.key] = 1;
        }
    }

    int lowest_key = 1;
    while (lowest_key <= max_key + 1)
    {
        if (used_keys[lowest_key] == 0)
        {
            break;
        }
        lowest_key++;
    }

    free(used_keys);
    close(fd_meta);

    return lowest_key;
}

int removeKey(int key)
{
    int fd_meta = open(INDEX_FILE, O_RDWR);
    if (fd_meta == -1)
        return -1;

    int fd_temp = open("temp_meta", O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd_temp == -1)
    {
        close(fd_meta);
        return -1;
    }

    ssize_t bytes_read;
    ArchiveMetadata entry;
    memset(&entry, 0, sizeof(ArchiveMetadata));

    int key_found = 0;

    while ((bytes_read = read(fd_meta, &entry, sizeof(ArchiveMetadata))) > 0)
    {
        if (entry.key != key)
        {
            write(fd_temp, &entry, sizeof(ArchiveMetadata));
        }
        else
        {
            key_found = 1;
        }
    }

    close(fd_meta);
    close(fd_temp);

    if (key_found)
    {
        if (remove(INDEX_FILE) == -1)
        {
            perror("Erro ao remover arquivo de metadados");
            return -1;
        }
        if (rename("temp_meta", INDEX_FILE) == -1)
        {
            perror("Erro ao renomear arquivo temporário");
            return -1;
        }
        return key;
    }
    else
    {
        remove("temp_meta");
        return -1;
    }
}

char *searchKey(int key)
{
    int fd_metadata = open(INDEX_FILE, O_RDONLY | O_CREAT, 0644);
    if (fd_metadata == -1)
    {
        perror("Erro ao abrir arquivo de metadados");
        char *buffer = (char *)malloc(BUFFER_SIZE);
        buffer[0] = '\0';
        return buffer;
    }

    char *buffer = (char *)malloc(BUFFER_SIZE);
    if (!buffer)
    {
        close(fd_metadata);
        return NULL;
    }

    buffer[0] = '\0';
    ArchiveMetadata doc;
    lseek(fd_metadata, 0, SEEK_SET);

    // Track the position of the most recent entry with the key
    off_t latest_pos = -1;
    off_t current_pos = 0;

    while (read(fd_metadata, &doc, sizeof(ArchiveMetadata)) > 0)
    {
        if (doc.key == key)
        {
            latest_pos = current_pos;
        }
        current_pos = lseek(fd_metadata, 0, SEEK_CUR);
    }

    // If found, read the latest entry
    if (latest_pos >= 0)
    {
        lseek(fd_metadata, latest_pos, SEEK_SET);
        read(fd_metadata, &doc, sizeof(ArchiveMetadata));

        char title[210];
        char authors[212];
        char year[32];
        char path[128];
        snprintf(title, sizeof(title), "Titulo: %s\n", doc.title);
        snprintf(authors, sizeof(authors), "Autores: %s\n", doc.authors);
        snprintf(year, sizeof(year), "Ano: %s\n", doc.year);
        snprintf(path, sizeof(path), "Caminho: %s\n", doc.path);

        strcat(buffer, title);
        strcat(buffer, authors);
        strcat(buffer, year);
        strcat(buffer, path);

        Meta cache;
        update_access_time(cache, key);
    }

    close(fd_metadata);
    return buffer;
}

int create_key()
{
    return find_lowest_available_key();
}

int check_existing_document(const char *title, const char *authors, const char *year, const char *path)
{
    int fd_metadata = open(INDEX_FILE, O_RDONLY | O_CREAT, 0644);
    if (fd_metadata == -1)
        return -1;

    ArchiveMetadata doc;
    lseek(fd_metadata, 0, SEEK_SET);
    printf("Verificando documento: title=%s, authors=%s, year=%s, path=%s\n",
           title, authors, year, path);

    while (read(fd_metadata, &doc, sizeof(ArchiveMetadata)) > 0)
    {
        if (strcmp(doc.title, title) == 0 &&
            strcmp(doc.authors, authors) == 0 &&
            strcmp(doc.year, year) == 0 &&
            strcmp(doc.path, path) == 0)
        {
            printf("Documento encontrado: key=%d\n", doc.key);
            close(fd_metadata);
            return doc.key;
        }
    }

    close(fd_metadata);
    return -1;
}

void append_to_file(int key, const char *title, const char *authors, const char *year, const char *path)
{
    int fd_metadata = open(INDEX_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd_metadata == -1)
    {
        perror("Erro ao abrir arquivo de metadados");
        return;
    }

    ArchiveMetadata new_doc = {0};
    new_doc.key = key;
    strcpy(new_doc.title, title);
    strcpy(new_doc.authors, authors);
    strcpy(new_doc.year, year);
    strcpy(new_doc.path, path);
    new_doc.last_access = time(NULL);

    printf("Adicionando documento: key=%d, title=%s, authors=%s, year=%s, path=%s\n",
           new_doc.key, new_doc.title, new_doc.authors, new_doc.year, new_doc.path);

    if (write(fd_metadata, &new_doc, sizeof(ArchiveMetadata)) == -1)
    {
        perror("Erro ao escrever no arquivo");
    }
    else
    {
        printf("Documento adicionado (key=%d): %s\n", key, title);
    }

    close(fd_metadata);

    if (key > global_key)
    {
        global_key = key;
    }
}

int fileToCache(Meta cache, int cache_size)
{
    int fd = open(INDEX_FILE, O_RDONLY | O_CREAT, 0644);
    if (fd == -1)
    {
        perror("Erro ao abrir arquivo de metadados");
        return -1;
    }

    // Lista temporária usando ArchiveMetadata
    ArchiveMetadata *head = NULL;
    ArchiveMetadata doc;
    int entry_count = 0;

    // Ler todas as entradas
    // Dentro da leitura dos registos:
    while (read(fd, &doc, sizeof(ArchiveMetadata)) > 0)
    {
        ArchiveMetadata *new_node = malloc(sizeof(ArchiveMetadata));
        memcpy(new_node, &doc, sizeof(ArchiveMetadata));
        new_node->next = NULL;

        // Inserir ordenado por last_access (mais recente primeiro)
        if (!head || new_node->last_access >= head->last_access)
        {
            new_node->next = head;
            head = new_node;
        }
        else
        {
            ArchiveMetadata *current = head;
            while (current->next && current->next->last_access > new_node->last_access)
            {
                current = current->next;
            }
            new_node->next = current->next;
            current->next = new_node;
        }
    }
    close(fd);

    // Preencher cache
    int count = 0;
    ArchiveMetadata *current = head;
    while (current && count < cache_size)
    {
        int index = current->key % cache_size;

        // Tratar colisões
        if (cache[index].key == 0)
        {
            memcpy(&cache[index], current, sizeof(ArchiveMetadata));
            cache[index].next = NULL;
        }
        else
        {
            // Inserir no início da lista encadeada (mais recente primeiro)
            ArchiveMetadata *new_node = malloc(sizeof(ArchiveMetadata));
            memcpy(new_node, &cache[index], sizeof(ArchiveMetadata)); // Copiar o antigo cabeça
            memcpy(&cache[index], current, sizeof(ArchiveMetadata));  // Novo cabeça
            cache[index].next = new_node;                             // Antigo cabeça agora é o segundo
        }
        count++;
        current = current->next;
    }

    // Libertar memória da lista temporária
    while (head)
    {
        ArchiveMetadata *temp = head;
        head = head->next;
        free(temp);
    }

    return count;
}

int indexMeta(Meta cache, char *title, char *authors, char *year, char *path, int key)
{
    if (cache[key].key == 0)
    {
        strcpy(cache[key].title, title);
        strcpy(cache[key].authors, authors);
        strcpy(cache[key].year, year);
        strcpy(cache[key].path, path);
        cache[key].key = key;
        cache[key].next = NULL;
        cache[key].last_access = time(NULL);
        return key;
    }
    if (cache[key].key == key)
    {
        update_access_time(cache, key);
        return 0;
    }

    ArchiveMetadata *current = &cache[key];
    while (current != NULL)
    {
        if (current->key == key)
        {
            update_access_time(cache, key);
            return 0;
        }
        current = current->next;
    }
    ArchiveMetadata *new = (ArchiveMetadata *)malloc(sizeof(ArchiveMetadata));
    strcpy(new->title, title);
    strcpy(new->authors, authors);
    strcpy(new->year, year);
    strcpy(new->path, path);
    new->key = key;
    new->next = NULL;
    new->last_access = time(NULL);
    printf("Adicionando documento à cache: key=%d, title=%s, authors=%s, year=%s, path=%s\n",
           new->key, new->title, new->authors, new->year, new->path);

    return key;
}

int apagaMeta(Meta cache, int key)
{
    if (cache[key].key == key)
    {
        if (cache[key].next == NULL)
        {
            memset(&cache[key], 0, sizeof(ArchiveMetadata));
        }
        else
        {
            ArchiveMetadata *temp = cache[key].next;
            cache[key].key = temp->key;
            strcpy(cache[key].title, temp->title);
            strcpy(cache[key].authors, temp->authors);
            strcpy(cache[key].year, temp->year);
            strcpy(cache[key].path, temp->path);
            cache[key].next = temp->next;
            free(temp);
        }
        return key;
    }
    ArchiveMetadata *current = cache[key].next;
    ArchiveMetadata *previous = &cache[key];
    while (current != NULL)
    {
        if (current->key == key)
        {
            previous->next = current->next;
            free(current);
            return key;
        }
        previous = current;
        current = current->next;
    }
    return -1;
}

int searchKeyWords(Meta cache, int key, char *palavra, char *palavra2)
{
    int ret = -1;
    if (cache[key].key == 0)
    {
        printf("Documento não encontrado na cache\n");
    }
    else
    {
        ArchiveMetadata *current = &cache[key];
        while (current != NULL)
        {
            if (current->key == key)
            {
                // Atualiza timestamp de acesso (LRU)
                current->last_access = time(NULL);

                size_t size = strlen(palavra2) + strlen(current->path) + 1;
                char *path = malloc(size);
                if (!path)
                {
                    perror("Erro de alocação de memória");
                    return -1;
                }

                path[0] = '\0';
                strcat(path, palavra2);
                strcat(path, current->path);

                ret = findKey(path, palavra);
                free(path);
                break;
            }
            current = current->next;
        }
    }

    if (ret == -1)
    {
        int fd = open(INDEX_FILE, O_RDWR);
        if (fd == -1)
        {
            perror("Erro ao abrir arquivo de metadados");
            return -1;
        }
        ArchiveMetadata doc;
        memset(&doc, 0, sizeof(ArchiveMetadata));

        while ((read(fd, &doc, sizeof(ArchiveMetadata))) > 0)
        {
            if (doc.key == key)
            {
                // Fix: Ensure proper size and null-termination
                size_t size = strlen(palavra2) + strlen(doc.path) + 1;
                char *path = malloc(size);
                if (!path)
                {
                    perror("Erro de alocação de memória");
                    close(fd);
                    return -1;
                }

                // Initialize and concatenate safely
                path[0] = '\0';
                strcat(path, palavra2);
                strcat(path, doc.path);

                ret = findKey(path, palavra);
                free(path);
                close(fd);
                return ret;
            }
        }
        close(fd);
    }

    return ret;
}

int findKey(char *path, char *palavra)
{
    if (access(path, R_OK) != 0)
    {
        perror("Erro ao acessar o arquivo");
        return -1;
    }

    int fd[2];
    if (pipe(fd) == -1)
    {
        perror("Erro ao criar pipe");
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("Erro ao criar processo");
        close(fd[0]);
        close(fd[1]);
        return -1;
    }

    if (pid == 0) // Processo filho
    {
        close(fd[0]);

        int grep_wc[2];
        if (pipe(grep_wc) == -1)
        {
            perror("Erro ao criar pipe interno");
            exit(EXIT_FAILURE);
        }

        pid_t child_pid = fork();
        if (child_pid == -1)
        {
            perror("Erro ao criar processo filho interno");
            exit(EXIT_FAILURE);
        }

        if (child_pid == 0) // Processo Neto
        {
            close(fd[0]);

            close(fd[1]);
            close(grep_wc[0]);

            dup2(grep_wc[1], STDOUT_FILENO);
            close(grep_wc[1]);

            execlp("grep", "grep", palavra, path, NULL);
            perror("Erro ao executar grep");
            exit(EXIT_FAILURE);
        }
        else // Processo Filho
        {
            close(grep_wc[1]);

            dup2(grep_wc[0], STDIN_FILENO);
            close(grep_wc[0]);

            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);

            execlp("wc", "wc", "-l", NULL);
            perror("Erro ao executar wc");
            exit(EXIT_FAILURE);
        }
    }

    // Processo pai
    close(fd[1]);

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(fd[0], buffer, sizeof(buffer) - 1);
    close(fd[0]);

    if (bytes_read <= 0)
    {
        return -1;
    }

    buffer[bytes_read] = '\0';
    return atoi(buffer);
}

// Função para atualizar o timestamp de acesso de um documento
void update_access_time(Meta cache, int key)
{
    // Verifica se o documento está na posição direta da tabela hash
    if (cache[key].key == key)
    {
        cache[key].last_access = time(NULL);
        return;
    }

    // Procura na lista encadeada se existir
    if (cache[key].key != 0)
    {
        ArchiveMetadata *current = cache[key].next;
        while (current != NULL)
        {
            if (current->key == key)
            {
                current->last_access = time(NULL);
                return;
            }
            current = current->next;
        }
    }
}

int find_lru_entry(Meta cache, int cache_size)
{
    long oldest_time = time(NULL);
    int lru_index = -1;

    for (int i = 0; i < cache_size; i++)
    {
        if (cache[i].key != 0)
        {
            // Verifica a entrada principal
            if (cache[i].last_access < oldest_time)
            {
                oldest_time = cache[i].last_access;
                lru_index = i;
            }

            // Verifica entradas encadeadas
            ArchiveMetadata *current = cache[i].next;
            while (current != NULL)
            {
                if (current->last_access < oldest_time)
                {
                    oldest_time = current->last_access;
                    lru_index = i; // Mantém o índice da cache, não a chave!
                }
                current = current->next;
            }
        }
    }

    return lru_index; // Retorna o ÍNDICE da cache, não a chave
}

void print_ocupados(Meta tabela)
{
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        if (tabela[i].key != 0)
        {
            printf("Índice %d:\n", i);
            printf("  Chave: %d\n", tabela[i].key);
            printf("  Título: %s\n", tabela[i].title);
            printf("  Autores: %s\n", tabela[i].authors);
            printf("  Ano: %s\n", tabela[i].year);
            printf("  Path: %s\n", tabela[i].path);

            ArchiveMetadata *atual = tabela[i].next;
            while (atual != NULL)
            {
                printf("    → Encadeado:\n");
                printf("      Chave: %d\n", atual->key);
                printf("      Título: %s\n", atual->title);
                printf("      Autores: %s\n", atual->authors);
                printf("      Ano: %s\n", atual->year);
                printf("      Path: %s\n", atual->path);
                atual = atual->next;
            }
            printf("\n");
        }
    }
}