#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
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

int removeKey(Meta cache, int key, int ind, int indexed_files)
{
    int res = -1;
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

    if (ind == -1)
    {
        while ((bytes_read = read(fd_meta, &entry, sizeof(ArchiveMetadata))) > 0)
        {
            if (entry.key != key)
            {
                write(fd_temp, &entry, sizeof(ArchiveMetadata));
            }
        }
    }
    else
    {
        ArchiveMetadata tmp;
        memset(&tmp, 0, sizeof(ArchiveMetadata));
        while ((bytes_read = read(fd_meta, &tmp, sizeof(ArchiveMetadata))) > 0)
        {
            if (entry.key != key)
            {
                if (entry.key == ind)
                {
                    if (tmp.contador < entry.key)
                    {
                        memcpy(&tmp, &entry, sizeof(ArchiveMetadata));
                    }
                }
                write(fd_temp, &tmp, sizeof(ArchiveMetadata));
            }
        }
        res = indexMeta(cache, tmp.title, tmp.authors, tmp.year, tmp.path, tmp.key);
        if (res == 0)
        {
            indexed_files--;
        }
    }
    close(fd_meta);
    close(fd_temp);
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
    return res;
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
    ArchiveMetadata doc;
    memset(&doc, 0, sizeof(ArchiveMetadata));

    int fd = open(INDEX_FILE, O_RDONLY | O_CREAT);
    if (fd == -1)
    {
        perror("Erro ao abrir arquivo de metadados");
        return -1;
    }
    int count = 0;
    ssize_t bytes_read;
    while ((bytes_read = read(fd, &doc, sizeof(ArchiveMetadata))) > 0)
    {
        if (count > cache_size)
        {
            break;
        }

        if (cache[doc.key].key == 0)
        {
            memcpy(&cache[doc.key], &doc, sizeof(ArchiveMetadata));
            cache[doc.key].next = NULL;
            count++;
            continue;
        }

        ArchiveMetadata *current = &cache[doc.key];
        ArchiveMetadata *previous = current;
        while (current->next != NULL)
        {
            if (current->key == doc.key)
            {
                printf("Documento já existe na cache\n");
                close(fd);
                break;
            }
            previous = current;
            current = current->next;
        }

        ArchiveMetadata *new = (ArchiveMetadata *)malloc(sizeof(ArchiveMetadata));
        strcpy(new->title, doc.title);
        strcpy(new->authors, doc.authors);
        strcpy(new->year, doc.year);
        strcpy(new->path, doc.path);
        new->key = doc.key;
        new->next = NULL;
        new->contador = doc.contador;
        previous->next = new;
        count++;
        printf("Adicionando documento à cache: key=%d, title=%s, authors=%s, year=%s, path=%s\n",
               new->key, new->title, new->authors, new->year, new->path);
    }
    close(fd);
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
        cache[key].contador = 0;
        cache[key].next = NULL;
    }
    else
    {
        ArchiveMetadata *current = &cache[key];
        ArchiveMetadata *previous = current;
        while (current != NULL)
        {
            if (current->key == key)
            {
                printf("Documento já existe na cache\n");
                return 0;
            }
            previous = current;
            current = current->next;
        }
        ArchiveMetadata *new = (ArchiveMetadata *)malloc(sizeof(ArchiveMetadata));
        strcpy(new->title, title);
        strcpy(new->authors, authors);
        strcpy(new->year, year);
        strcpy(new->path, path);
        new->key = key;
        new->next = NULL;
        new->contador = 0;
        previous->next = new;
        printf("Adicionando documento à cache: key=%d, title=%s, authors=%s, year=%s, path=%s\n",
               new->key, new->title, new->authors, new->year, new->path);
    }
    return key;
}

int apagaMeta(Meta cache, int key)
{
    if (cache[key].key == 0)
    {
        printf("Documento não encontrado na cache\n");
        return -1;
    }
    else
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
    }
    return -1;
}

void print_ocupados(Meta tabela)
{
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        if (tabela[i].key != 0)
        {
            printf("Índice %d:\n", i);
            printf("  Chave: %ld\n", tabela[i].key);
            printf("  Título: %s\n", tabela[i].title);
            printf("  Autores: %s\n", tabela[i].authors);
            printf("  Ano: %s\n", tabela[i].year);
            printf("  Path: %s\n", tabela[i].path);
            printf("  Contador: %d\n", tabela[i].contador);

            ArchiveMetadata *atual = tabela[i].next;
            while (atual != NULL)
            {
                printf("    → Encadeado:\n");
                printf("      Chave: %ld\n", atual->key);
                printf("      Título: %s\n", atual->title);
                printf("      Autores: %s\n", atual->authors);
                printf("      Ano: %s\n", atual->year);
                printf("      Path: %s\n", atual->path);
                printf("      Contador: %d\n", atual->contador);
                atual = atual->next;
            }
            printf("\n");
        }
    }
}