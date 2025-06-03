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
        fprintf(stderr, "Erro: Nenhum comando dado\n");
        return 1;
    }

    DocumentMetadata doc;
    memset(&doc, 0, sizeof(DocumentMetadata));

    if (strcmp(argv[1], "-f") == 0)
    {
        if (argc != 2)
        {
            fprintf(stderr, "Erro: Número de argumentos para -f inválido\n");
            return 1;
        }
        strcpy(doc.flag, "-f");
    }
    else if (strcmp(argv[1], "-a") == 0)
    {
        if (argc != 6)
        {
            fprintf(stderr, "Erro: Número de argumentos para -a inválido\n");
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
            fprintf(stderr, "Erro: Número de argumentos para -d inválido\n");
            return 1;
        }
        strcpy(doc.flag, "-d");
        doc.key = atoi(argv[2]);
    }
    else if (strcmp(argv[1], "-c") == 0)
    {
        if (argc != 3)
        {
            fprintf(stderr, "Erro: Número de argumentos para -c inválido\n");
            return 1;
        }
        strcpy(doc.flag, "-c");
        doc.key = atoi(argv[2]);
    }
    else if (strcmp(argv[1], "-l") == 0)
    {
        if (argc != 4)
        {
            fprintf(stderr, "Erro: Número de argumentos para -l inválido\n");
            return 1;
        }
        strcpy(doc.flag, "-l");
        doc.key = atoi(argv[2]);
        strcpy(doc.palavra, argv[3]);
    }
    else if (strcmp(argv[1], "-s") == 0)
    {
        if (argc != 4)
        {
            fprintf(stderr, "Erro: Número de argumentos para -s inválido\n");
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

    // Abrir pipe para escrita
    int write_fd = open(CLIENT_PIPE, O_WRONLY);
    if (write_fd == -1) {
        perror("Erro ao abrir pipe do cliente. O servidor está a rodar?");
        return 1;
    }

    // Enviar comando
    ssize_t bytes_written = write(write_fd, &doc, sizeof(DocumentMetadata));
    if (bytes_written == -1)
    {
        perror("Erro ao escrever no pipe do cliente");
        close(write_fd);
        return 1;
    }
    close(write_fd);
    
    // Abrir o pipe para leitura
    int read_fd = open(SERVER_PIPE, O_RDONLY);
    if (read_fd == -1) {
        perror("Erro ao abrir pipe do servidor para leitura");
        return 1;
    }

    // Ler resposta
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(read_fd, buffer, BUFFER_SIZE - 1);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Recebido pelo servidor:\n%s", buffer);
    } else if (bytes_read == 0) {
        printf("Resposta não obtida\n");
    } else {
        perror("Erro a ler do pipe do servidor");
    }
    
    close(read_fd);
    return 0;
}