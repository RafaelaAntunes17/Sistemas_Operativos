#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include "../include/server.h"
#include "../include/defs.h"

int global_key = 0;

char *searchKey(int key)
{
    int fd_metadata = open(METADATA_FILE, O_RDONLY | O_CREAT, 0644);
    if (fd_metadata == -1)
    {
        perror("Erro ao abrir arquivo de metadados");
        return NULL;
    }

    char *buffer = (char *)malloc(BUFFER_SIZE);
    buffer[0] = '\0';
    ArchiveMetadata doc;
    lseek(fd_metadata, 0, SEEK_SET);

    while (read(fd_metadata, &doc, sizeof(ArchiveMetadata)) > 0)
    {
        if (doc.key == key)
        {
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

            return buffer;
        }
    }

    close(fd_metadata);
    return NULL; // Retorna NULL se não encontrar o documento
}

int create_key()
{
    return global_key++;
}

int check_existing_document(const char *title, const char *authors, const char *year, const char *path)
{
    int fd_metadata = open(METADATA_FILE, O_RDONLY | O_CREAT, 0644);
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
            return doc.key;  // Retorna a chave do documento encontrado
        }
    }

    close(fd_metadata);
    return -1; 
}

void append_to_metadata(int key, const char *title, const char *authors, const char *year, const char *path)
{
    int fd_metadata = open(METADATA_FILE, O_RDWR | O_CREAT, 0644);
    if (fd_metadata == -1)
    {
        perror("Erro ao abrir arquivo de metadados");
        return;
    }

    lseek(fd_metadata, 0, SEEK_SET); // Posiciona no início do arquivo

    ArchiveMetadata new_doc = {0};
    new_doc.key = key;
    strcpy(new_doc.title, title);
    strcpy(new_doc.authors, authors);
    strcpy(new_doc.year, year);
    strcpy(new_doc.path, path);

    printf("Adicionando documento: key=%d, title=%s, authors=%s, year=%s, path=%s\n",
           new_doc.key, new_doc.title, new_doc.authors, new_doc.year, new_doc.path);

    lseek(fd_metadata, 0, SEEK_END);
    if (write(fd_metadata, &new_doc, sizeof(ArchiveMetadata)) == -1)
    {
        perror("Erro ao escrever no arquivo");
    }
    else
    {
        printf("Documento adicionado (key=%d): %s\n", key, title);
    }

    close(fd_metadata);
}