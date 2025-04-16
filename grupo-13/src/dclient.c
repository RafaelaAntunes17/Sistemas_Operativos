#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "defs.h"

#define MAXBUFFER 254
#define MAX_DOCUMENTS 100

DocumentList docList;

void indexação(char **argv)
{
    char *title = argv[2];
    char *authors = argv[3];
    char *year = argv[4];
    char *path = argv[5];

    if (docList.count >= MAX_DOCUMENTS)
    {
        perror("Document list is full");
        return;
    }

    int fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        perror("Error opening file");
        return;
    }

    char *file_content = malloc(MAXBUFFER);
    if (file_content == NULL)
    {
        perror("Error allocating memory");
        close(fd);
        return;
    }

    __ssize_t total_size = 0;
    __ssize_t bytesread;
    while ((bytesread = read(fd, file_content + total_size, MAXBUFFER)) > 0)
    {
        total_size += bytesread;

        char *temp = realloc(file_content, total_size + MAXBUFFER);
        if (temp == NULL)
        {
            perror("Error reallocating memory");
            free(file_content);
            close(fd);
            return;
        }
        file_content = temp;
    }
    if (bytesread == -1)
    {
        perror("Error reading file");
        free(file_content);
        close(fd);
        return;
    }
    close(fd);

    file_content[total_size] = '\0';

    Document *doc = &docList.documents[docList.count];
    strncpy(doc->title, title, sizeof(doc->title) - 1);
    strncpy(doc->authors, authors, sizeof(doc->authors) - 1);
    strncpy(doc->year, year, sizeof(doc->year) - 1);
    strncpy(doc->path, path, sizeof(doc->path) - 1);

    free(file_content);

    char message[50];
    docList.count++;
    snprintf(message, sizeof(message), "Document %d indexed\n", docList.count);
    write(1, message, strlen(message)); // Placeholder for document ID
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        perror("Missing arguments");
        return -1;
    }
    return 0;
}