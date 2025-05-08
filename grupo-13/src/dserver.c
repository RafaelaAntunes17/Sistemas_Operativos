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

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Uso: %s document_folder cache_size\n", argv[0]);
        return EXIT_FAILURE;
    }

    int cache_size = atoi(argv[2]);
    if (cache_size <= 0)
    {
        fprintf(stderr, "Erro: cache_size deve ser positivo\n");
        return EXIT_FAILURE;
    }

    unlink(SERVER_PIPE);
    unlink(CLIENT_PIPE);

    // Criar novos FIFOs
    if (mkfifo(SERVER_PIPE, 0666) == -1)
    {
        perror("Erro ao criar pipe do servidor");
        return EXIT_FAILURE;
    }

    if (mkfifo(CLIENT_PIPE, 0666) == -1)
    {
        perror("Erro ao criar pipe de respostas");
        return EXIT_FAILURE;
    }

    printf("Servidor iniciado. Aguardando comandos...\n");
    // Inicializar o estado do servidor
    int server_fd = open(SERVER_PIPE, O_RDONLY);
    if (server_fd == -1)
    {
        perror("Erro ao abrir pipe do servidor");
        return EXIT_FAILURE;
    }

    // Abrir pipe do servidor
    int client_fd = open(CLIENT_PIPE, O_WRONLY);
    if (client_fd == -1)
    {
        perror("Erro ao abrir pipe de respostas");
        return EXIT_FAILURE;
    }
    // Loop principal do servidor
    ssize_t bytes_read;

    while (1)
    {
        DocumentMetadata doc;

        while ((bytes_read = read(server_fd, &doc, sizeof(DocumentMetadata))))
        {
            if (bytes_read == -1)
            {
                perror("Erro ao ler do pipe do servidor");
                close(server_fd);
                close(client_fd);
                unlink(SERVER_PIPE);
                unlink(CLIENT_PIPE);
                return EXIT_FAILURE;
            }
            if (strcmp(doc.flag, "-f") == 0)
            {
                const char *msg = "Servidor a encerrar...\n";
                write(server_fd, msg, strlen(msg));
                close(server_fd);
                close(client_fd);
                unlink(SERVER_PIPE);
                unlink(CLIENT_PIPE);
                printf("Servidor encerrado.\n");
                return EXIT_SUCCESS;
            }
        }
    }

    // Limpeza final
    close(server_fd);
    close(client_fd);
    unlink(SERVER_PIPE);
    unlink(CLIENT_PIPE);

    printf("Servidor encerrado.\n");
    return EXIT_SUCCESS;
}