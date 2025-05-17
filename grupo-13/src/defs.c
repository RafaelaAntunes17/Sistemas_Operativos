#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
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

    // Inicializar a cache
    for (int i = 0; i < cache_size; i++)
    {
        cache[i].key = 0;
        cache[i].next = NULL;
    }

    // Array para armazenar temporariamente os documentos
    ArchiveMetadata *temp_docs = NULL;
    int doc_count = 0;

    ArchiveMetadata temp;
    doc_count = 0;
    lseek(fd, 0, SEEK_SET);

    // Conta quantos registros existem lendo um por um
    while (read(fd, &temp, sizeof(ArchiveMetadata)) == sizeof(ArchiveMetadata))
    {
        doc_count++;
    }

    if (doc_count == 0)
    {
        close(fd);
        return 0;
    }

    temp_docs = (ArchiveMetadata *)malloc(doc_count * sizeof(ArchiveMetadata));
    if (!temp_docs)
    {
        perror("Erro de alocação de memória");
        close(fd);
        return -1;
    }

    lseek(fd, 0, SEEK_SET);
    ssize_t bytes_read = read(fd, temp_docs, doc_count * sizeof(ArchiveMetadata));
    close(fd);

    if (bytes_read < 0)
    {
        perror("Erro ao ler documentos");
        free(temp_docs);
        return -1;
    }

    doc_count = bytes_read / sizeof(ArchiveMetadata);

    // Ordenar por timestamp (LRU)
    for (int i = 0; i < doc_count - 1; i++)
    {
        for (int j = 0; j < doc_count - i - 1; j++)
        {
            if (temp_docs[j].last_access < temp_docs[j + 1].last_access)
            {
                // Trocar documentos
                ArchiveMetadata temp = temp_docs[j];
                temp_docs[j] = temp_docs[j + 1];
                temp_docs[j + 1] = temp;
            }
        }
    }

    int loaded_count = 0;

    // Agora carregamos na cache os mais recentemente acessados (até cache_size)
    for (int i = 0; i < doc_count && i < cache_size; i++)
    {
        cache[i].key = temp_docs[i].key;
        strcpy(cache[i].title, temp_docs[i].title);
        strcpy(cache[i].authors, temp_docs[i].authors);
        strcpy(cache[i].year, temp_docs[i].year);
        strcpy(cache[i].path, temp_docs[i].path);
        cache[i].last_access = temp_docs[i].last_access;
        cache[i].next = NULL;
        loaded_count++;
    }

    for (int i = cache_size; i < doc_count; i++)
    {
        int index = temp_docs[i].key % cache_size;

        ArchiveMetadata *new_node = (ArchiveMetadata *)malloc(sizeof(ArchiveMetadata));
        if (!new_node)
            continue;

        new_node->key = temp_docs[i].key;
        strcpy(new_node->title, temp_docs[i].title);
        strcpy(new_node->authors, temp_docs[i].authors);
        strcpy(new_node->year, temp_docs[i].year);
        strcpy(new_node->path, temp_docs[i].path);
        new_node->last_access = temp_docs[i].last_access;

        // Inserir na lista encadeada
        new_node->next = cache[index].next;
        cache[index].next = new_node;
        loaded_count++;
    }
    free(temp_docs);
    return loaded_count;
}

int indexMeta(Meta cache, char *title, char *authors, char *year, char *path, int key)
{
    int index = key % BUFFER_SIZE;

    // Se o slot estiver vazio, simplesmente adicionamos
    if (cache[index].key == 0)
    {
        strcpy(cache[index].title, title);
        strcpy(cache[index].authors, authors);
        strcpy(cache[index].year, year);
        strcpy(cache[index].path, path);
        cache[index].key = key;
        cache[index].next = NULL;
        cache[index].last_access = time(NULL);
        return key;
    }

    // Se o documento já existe na posição principal, apenas atualizamos o timestamp
    if (cache[index].key == key)
    {
        cache[index].last_access = time(NULL);
        return 0;
    }

    // Procuramos na lista encadeada
    ArchiveMetadata *current = &cache[index];
    while (current->next != NULL)
    {
        if (current->next->key == key)
        {
            current->next->last_access = time(NULL);
            return 0;
        }
        current = current->next;
    }

    // Não encontramos, então adicionamos no final da lista
    ArchiveMetadata *new_node = (ArchiveMetadata *)malloc(sizeof(ArchiveMetadata));
    strcpy(new_node->title, title);
    strcpy(new_node->authors, authors);
    strcpy(new_node->year, year);
    strcpy(new_node->path, path);
    new_node->key = key;
    new_node->next = NULL;
    new_node->last_access = time(NULL);

    current->next = new_node;

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
                size_t size = strlen(palavra2) + strlen(doc.path) + 1;
                char *path = malloc(size);
                if (!path)
                {
                    perror("Erro de alocação de memória");
                    close(fd);
                    return -1;
                }

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

char *searchAll(char *palavra, char *directory_path, int nr_processos)
{
    int fd_ind = open(INDEX_FILE, O_RDONLY);
    if (fd_ind == -1)
    {
        perror("Erro ao abrir arquivo de metadados");
        return NULL;
    }

    off_t file_size = lseek(fd_ind, 0, SEEK_END);
    int nr_files = file_size / sizeof(ArchiveMetadata);
    lseek(fd_ind, 0, SEEK_SET);
    int max = (nr_files / nr_processos) + 1;
    close(fd_ind);

    int pipes[nr_processos][2];
    pid_t pids[nr_processos];
    char *buffer = malloc(BUFFER_SIZE * nr_processos);
    buffer[0] = '\0';

    // Criar todos os pipes primeiro
    for (int i = 0; i < nr_processos; i++)
    {
        if (pipe(pipes[i]) == -1)
        {
            perror("Erro ao criar pipe");
            for (int j = 0; j < i; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            free(buffer);
            return NULL;
        }
    }

    for (int i = 0; i < nr_processos; i++)
    {
        pids[i] = fork();
        if (pids[i] == 0)
        { // Processo filho
            // Fechar pipes não utilizados
            for (int j = 0; j < nr_processos; j++)
            {
                if (j != i)
                {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
            }
            close(pipes[i][0]); // Fechar extremidade de leitura

            int fd_meta = open(INDEX_FILE, O_RDONLY);
            if (fd_meta == -1)
            {
                perror("Erro no filho ao abrir metadados");
                exit(EXIT_FAILURE);
            }

            ArchiveMetadata doc;
            off_t offset = i * max * sizeof(ArchiveMetadata);
            lseek(fd_meta, offset, SEEK_SET);

            char local_buffer[BUFFER_SIZE] = {0};

            for (int j = 0; j < max; j++)
            {
                ssize_t bytes_read = read(fd_meta, &doc, sizeof(ArchiveMetadata));
                if (bytes_read != sizeof(ArchiveMetadata))
                    break;

                char caminho[BUFFER_SIZE];
                snprintf(caminho, sizeof(caminho), "%s/%s", directory_path, doc.path);

                if (access(caminho, F_OK) == 0)
                { 
                    int ocorrencias = findKey(caminho, palavra);
                    if (ocorrencias > 0)
                    {
                        char entry[32];
                        snprintf(entry, sizeof(entry), "%d, ", doc.key);
                        strcat(local_buffer, entry);
                    }
                }
            }
            close(fd_meta);

            size_t len = strlen(local_buffer);
            if (len > 0)
                local_buffer[len - 2] = '\0';

            write(pipes[i][1], local_buffer, strlen(local_buffer));
            close(pipes[i][1]);
            exit(EXIT_SUCCESS);
        }
        else if (pids[i] > 0)
        {                       // Processo pai
            close(pipes[i][1]); // Fechar escrita no pai
        }
        else
        {
            perror("Fork falhou");
            free(buffer);
            return NULL;
        }
    }

    // Coletar resultados de todos os filhos
    for (int i = 0; i < nr_processos; i++)
    {
        waitpid(pids[i], NULL, 0); // Esperar filho terminar

        char temp[BUFFER_SIZE];
        ssize_t bytes_read = read(pipes[i][0], temp, sizeof(temp));
        if (bytes_read > 0)
        {
            temp[bytes_read] = '\0';
            if (buffer[0] != '\0')
                strcat(buffer, ", ");
            strcat(buffer, temp);
        }
        close(pipes[i][0]);
    }

    // Formatar resultado final
    if (buffer[0] == '\0')
    {
        free(buffer);
        return NULL;
    }
    else
    {
        char *final_result = malloc(strlen(buffer) + 4);
        sprintf(final_result, "[%s]\n", buffer);
        free(buffer);
        return final_result;
    }
}

void update_access_time(Meta cache, int key)
{
    // Atualiza na cache
    int cache_index = key % BUFFER_SIZE;
    int updated_in_cache = 0;

    // Verificar nó principal
    if (cache[cache_index].key == key)
    {
        cache[cache_index].last_access = time(NULL);
        updated_in_cache = 1;
    }
    else if (cache[cache_index].key != 0)
    {
        // Verificar nós encadeados
        ArchiveMetadata *current = cache[cache_index].next;
        while (current != NULL)
        {
            if (current->key == key)
            {
                current->last_access = time(NULL);
                updated_in_cache = 1;
                break;
            }
            current = current->next;
        }
    }

    int fd_meta = open(INDEX_FILE, O_RDWR);
    if (fd_meta == -1)
        return;

    ArchiveMetadata doc;
    while (read(fd_meta, &doc, sizeof(ArchiveMetadata)) > 0)
    {
        if (doc.key == key)
        {
            // Voltar para a posição deste registro
            lseek(fd_meta, -sizeof(ArchiveMetadata), SEEK_CUR);

            // Atualizar timestamp
            doc.last_access = time(NULL);

            // Escrever de volta
            write(fd_meta, &doc, sizeof(ArchiveMetadata));
            break;
        }
    }

    close(fd_meta);
}

int find_lru_entry(Meta cache, int cache_size)
{
    time_t oldest_time = time(NULL);
    int lru_index = -1;
    ArchiveMetadata *lru_node = NULL;

    // Primeiro encontramos a entrada LRU em toda a cache
    for (int i = 0; i < cache_size; i++)
    {
        if (cache[i].key != 0)
        {
            // Verifica a entrada principal
            if (cache[i].last_access < oldest_time)
            {
                oldest_time = cache[i].last_access;
                lru_index = i;
                lru_node = &cache[i];
            }

            // Verifica entradas encadeadas
            ArchiveMetadata *current = cache[i].next;
            while (current != NULL)
            {
                if (current->last_access < oldest_time)
                {
                    oldest_time = current->last_access;
                    lru_index = i;
                    lru_node = current;
                }
                current = current->next;
            }
        }
    }

    return lru_index;
}