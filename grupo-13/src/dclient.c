#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>


#define MAXBUFFER 254
#define MAX_DOCUMENTS 100

typedef struct
{
    char title[200];
    char authors[200];
    char year[5];
    char path[64];
} Document;

typedef struct
{
    Document documents[MAX_DOCUMENTS];
    int count;
} DocumentList;
DocumentList docList;

void indexação(char **argv)
{
    char *title = argv[2];
    char *authors = argv[3];
    char *year = argv[4];
    char *path = argv[5];

    if(docList.count >= MAX_DOCUMENTS)
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

void consulta(char **argv)
{
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: ./dclient <command> [options]\n");
        return 1;
    }

    if (strcmp(argv[1], "-a") == 0)
    {
        indexação(argv);
    }
    else if (strcmp(argv[1], "-c") == 0)
    {
        if (argc != 3)
        {
            fprintf(stderr, "Usage: ./dclient -c <document_id>\n");
            return 1;
        }
        consulta(argv);
    }
    else if (strcmp(argv[1], "-d") == 0)
    {
        if (argc != 3)
        {
            fprintf(stderr, "Usage: ./dclient -d <document_id>\n");
            return 1;
        }
    }
    else if (strcmp(argv[1], "-l") == 0)
    {
        if (argc != 4)
        {
            fprintf(stderr, "Usage: ./dclient -l <document_id> <keyword>\n");
            return 1;
        }
    }
    else if (strcmp(argv[1], "-s") == 0)
    {
        if (argc != 3 && argc != 4)
        {
            fprintf(stderr, "Usage: ./dclient -s <keyword> [process_count]\n");
            return 1;
        }
    }
    else if (strcmp(argv[1], "-f") == 0)
    {
        if (argc != 2)
        {
            fprintf(stderr, "Usage: ./dclient -f\n");
            return 1;
        }
    }
    else
    {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        return 1;
    }

    return 0;
}