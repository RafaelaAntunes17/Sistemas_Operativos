#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "../include/client.h"

void build_command(int argc, char *argv[], char *command)
{
    strcat(command, argv[1]);
    for (int i = 2; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            strcat(command, " ");
            strcat(command, argv[i]);
        }
        else
        {
            strcat(command, " \"");
            strcat(command, argv[i]);
            strcat(command, "\"");
        }
    }
}

void print_usage(void)
{
    printf("Uso: dclient [opção] [argumentos]\n");
    printf("Opções:\n");
    printf("  -a \"title\" \"authors\" \"year\" \"path\"  Adicionar documento\n");
    printf("  -c \"key\"                         Consultar documento\n");
    printf("  -d \"key\"                         Remover documento\n");
    printf("  -l \"key\" \"keyword\"               Contar linhas com palavra-chave\n");
    printf("  -s \"keyword\" [nr_processos]       Pesquisar documentos\n");
    printf("  -f                                Desligar servidor\n");
}

void display_document_info(const DocumentMetadata *doc)
{
    if (doc != NULL)
    {
        printf("Key: %d\n", doc->key);
        printf("Title: %s\n", doc->title);
        printf("Authors: %s\n", doc->authors);
        printf("Year: %s\n", doc->year);
        printf("Path: %s\n", doc->path);
    }
    else
    {
        printf("Documento não encontrado.\n");
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        perror("Error: No command provided");
        return 1;
    }

    char command[BUFFER_SIZE] = {0};
    build_command(argc, argv, command);

    int server_fd = open(SERVER_PIPE, O_WRONLY);
    if (server_fd == -1)
    {
        perror("Error opening server FIFO");
        return 1;
    }
    if (write(server_fd, command, strlen(command)) == -1)
    {
        perror("Error writing to server FIFO");
        close(server_fd);
        return 1;
    }
    close(server_fd);

    if (strcmp(argv[1], "-f") != 0)
    {
        int response_fd = open(CLIENT_PIPE, O_RDONLY);
        if (response_fd == -1)
        {
            perror("Error opening client FIFO");
            return 1;
        }
        char response[BUFFER_SIZE];
        ssize_t bytes_read = read(response_fd, response, sizeof(response) - 1);
        if (bytes_read > 0)
        {
            response[bytes_read] = '\0';
            printf("Response from server: %s\n", response);
        }
        else if (bytes_read == -1)
        {
            perror("Error reading from client FIFO");
        }
        close(response_fd);
    }
    return 0;
}
