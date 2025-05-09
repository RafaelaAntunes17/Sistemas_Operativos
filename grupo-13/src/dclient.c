#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "../include/server.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        perror("Error: No command provided");
        return 1;
    }

    DocumentMetadata doc;
    memset(&doc, 0, sizeof(DocumentMetadata));

    if (strcmp(argv[1], "-f") == 0)
    {
        if (argc != 2)
        {
            perror("Error: Invalid number of arguments for -f");
            return 1;
        }
        strcpy(doc.flag, "-f");
    }
    else if (strcmp(argv[1], "-a") == 0)
    {
        if (argc != 5)
        {
            perror("Error: Invalid number of arguments for -a");
            return 1;
        }
        strcpy(doc.flag, "-a");
        strcpy(doc.title, argv[2]);
        strcpy(doc.authors, argv[3]);
        strcpy(doc.year, argv[4]);
        strcpy(doc.path, argv[5]);
    }
    else if (strcmp(argv[1], "-d") == 0)
    {
        if (argc != 3)
        {
            perror("Error: Invalid number of arguments for -d");
            return 1;
        }
        strcpy(doc.flag, "-d");
        doc.key = atoi(argv[2]);
    }
    else if (strcmp(argv[1], "-c") == 0)
    {
        if (argc != 3)
        {
            perror("Error: Invalid number of arguments for -d");
            return 1;
        }
        strcpy(doc.flag, "-c");
        doc.key = atoi(argv[2]);
    }
    else if (strcmp(argv[1], "-l") == 0)
    {
        if (argc != 4)
        {
            perror("Error: Invalid number of arguments for -l");
            return 1;
        }
        strcpy(doc.flag, "-l");
        doc.key = atoi(argv[2]);
        strcpy(doc.palavra, argv[3]);
    }
    else if (strcmp(argv[1], "-s") == 0)
    {
        if (argc != 3)
        {
            perror("Error: Invalid number of arguments for -s");
            return 1;
        }
        strcpy(doc.flag, "-s");
        strcpy(doc.palavra, argv[2]);
        doc.nr_procuras = atoi(argv[3]);
    }
    else
    {
        printf("Comando não reconhecido\n");
        return 1;
    }

    int write_fd = open(SERVER_PIPE, O_WRONLY);
    if (write_fd == -1)
    {
        perror("Error opening client pipe");
        return 1;
    }

    ssize_t bytes_written = write(write_fd, &doc, sizeof(DocumentMetadata));
    if (bytes_written == -1)
    {
        perror("Error writing to client pipe");
        close(write_fd);
        return 1;
    }
    printf("Command sent to server: %s\n", doc.flag);

    int read_fd = open(CLIENT_PIPE, O_RDONLY);
    if (read_fd == -1)
    {
        perror("Error opening server pipe");
        return 1;
    }
    int document_count = 0;
    if (read(read_fd, &document_count, sizeof(int)) == -1)
    {
        perror("Error reading document count");
        close(read_fd);
        return 1;
    }
    close(write_fd);
    char buffer[BUFFER_SIZE];
    ssize_t total_bytes;
    while (total_bytes < document_count)
    {
        ssize_t bytes_lidos = read(read_fd, buffer, sizeof(buffer));
        if (bytes_lidos == -1)
        {
            perror("Error reading from server pipe");
            close(read_fd);
            return 1;
        }
        buffer[bytes_lidos] = '\0'; // Null-terminate the string
        printf("Received from server: %s\n", buffer);
        total_bytes += bytes_lidos;
    }
    close(read_fd);

    return 0;
}
