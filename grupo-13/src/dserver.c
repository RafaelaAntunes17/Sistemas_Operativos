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

void append_to_metadata(const char *title, const char *authors, const char *year, const char *path) {
    int fd = open(METADATA_FILE, O_WRONLY | O_APPEND | O_CREAT, 0666);
    if (fd == -1) {
        perror("Erro a abrir o ficheiro");
        return;
    }

    size_t line_len = strlen(title) + strlen(authors) + strlen(year) + strlen(path) + 4;
    char *line = malloc(line_len + 1);

    if (!line) {
        perror("Erro na alocação da memória");
        close(fd);
        return;
    }

    snprintf(line, line_len + 1, "%s;%s;%s;%s\n", title, authors, year, path);

    ssize_t bytes_written = write(fd, line, line_len);
    
    if (bytes_written == -1) {
        perror("Erro na escrita");
    } 
    else if ((size_t)bytes_written != line_len) {
        fprintf(stderr, "Erro no texto escrito\n");
    }

    free(line);
    close(fd);
}

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

    while (1) {
        char cmd_buffer[BUFFER_SIZE];
        ssize_t bytes_read = read(server_fd, cmd_buffer, BUFFER_SIZE);

        if (bytes_read > 0) {
            char *flag = strtok(cmd_buffer, "|");
            if (strcmp(flag, "-f") == 0) {
                // Handle server shutdown
                break;
            } else if (strcmp(flag, "-a") == 0) {
                char *title = strtok(NULL, "|");
                char *authors = strtok(NULL, "|");
                char *year = strtok(NULL, "|");
                char *path = strtok(NULL, "|");
                append_to_metadata(title, authors, year, path);
            } else if (strcmp(flag, "-d") == 0) {
                int key = atoi(strtok(NULL, "|"));
                delete_from_metadata(key);
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