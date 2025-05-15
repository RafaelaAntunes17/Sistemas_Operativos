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
#include "../include/defs.h"

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
    Meta cache;
    memset(cache, 0, sizeof(cache));
    int indexed_files = 0;
    indexed_files = fileToCache(cache, cache_size);
    print_ocupados(cache);

    // Remover os pipes existentes
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
        unlink(SERVER_PIPE);
        return EXIT_FAILURE;
    }

    // Ler e inicializar metadados
    int fd_meta = open(INDEX_FILE, O_RDONLY | O_CREAT, 0644);
    if (fd_meta == -1)
    {
        perror("Erro ao abrir arquivo de metadados");
        unlink(SERVER_PIPE);
        unlink(CLIENT_PIPE);
        return EXIT_FAILURE;
    }

    ArchiveMetadata existing_doc;
    int max_key = 0;

    while (read(fd_meta, &existing_doc, sizeof(ArchiveMetadata)) > 0)
    {
        if (existing_doc.key > max_key)
        {
            max_key = existing_doc.key;
        }
    }

    global_key = max_key;
    close(fd_meta);

    printf("Valor máximo de key encontrado: %d\n", max_key);
    printf("Servidor iniciado. Aguardando comandos...\n");

    while (1)
    {
        printf("Aguardando comandos do cliente...\n");
        // Abrir os pipes com cada iteração para prevenir fechamentos inesperados
        int client_fd = open(CLIENT_PIPE, O_RDONLY);
        if (client_fd == -1)
        {
            perror("Erro ao abrir pipe do cliente");
            return EXIT_FAILURE;
        }

        int server_fd = open(SERVER_PIPE, O_WRONLY);
        if (server_fd == -1)
        {
            perror("Erro ao abrir pipe do servidor");
            close(client_fd);
            return EXIT_FAILURE;
        }

        // Ler comandos do cliente
        DocumentMetadata doc;
        ssize_t bytes_read;

        bytes_read = read(client_fd, &doc, sizeof(DocumentMetadata));
        if (bytes_read <= 0)
        {
            if (bytes_read == -1)
                perror("Erro ao ler do pipe do cliente");

            close(client_fd);
            close(server_fd);
            return EXIT_FAILURE;
        }

        // Processar comandos
        if (strcmp(doc.flag, "-f") == 0)
        {
            const char *msg = "Servidor a encerrar...\n";
            write(server_fd, msg, strlen(msg));
            close(client_fd);
            close(server_fd);
            unlink(SERVER_PIPE);
            unlink(CLIENT_PIPE);
            printf("Servidor encerrado.\n");
            return 0;
        }
        else if (strcmp(doc.flag, "-a") == 0)
        {

            int existing_key = check_existing_document(doc.title, doc.authors, doc.year, doc.path);
            if (existing_key > 0)
            {
                char msg[BUFFER_SIZE];
                snprintf(msg, sizeof(msg), "Documento %d já indexado \n", existing_key);
                write(server_fd, msg, strlen(msg));
            }
            else
            {
                int new_key = create_key();
                if (indexed_files < cache_size)
                {
                    append_to_file(new_key, doc.title, doc.authors, doc.year, doc.path);
                    indexMeta(cache, doc.title, doc.authors, doc.year, doc.path, new_key);
                    indexed_files++;
                }
                else
                {
                    append_to_file(new_key, doc.title, doc.authors, doc.year, doc.path);
                    indexed_files++;
                }
                char msg[BUFFER_SIZE];
                snprintf(msg, sizeof(msg), "Documento %d indexado\n", new_key);
                write(server_fd, msg, strlen(msg));
            }
        }
        else if (strcmp(doc.flag, "-c") == 0)
        {
            pid_t pid = fork();
            if (pid == 0)
            {
                char *result = searchKey(doc.key);
                if (*result == '\0')
                {
                    char msg[BUFFER_SIZE];
                    snprintf(msg, sizeof(msg), "Documento não encontrado (key=%d)\n", doc.key);
                    write(server_fd, msg, strlen(msg));
                }
                else
                {
                    write(server_fd, result, strlen(result));
                }
                free(result);
                exit(0);
            }
            else
            {
                close(client_fd);
                printf("Procura em PID = %d\n", pid);
            }
        }
        else if (strcmp(doc.flag, "-d") == 0)
        {
            int ind;
            pid_t pid = fork();
            if (pid == 0)
            {
                if((ind = apagaMeta(cache, doc.key)) == -1)
                {
                    char msg[BUFFER_SIZE];
                    snprintf(msg, sizeof(msg), "Documento não encontrado (key=%d)\n", doc.key);
                    write(server_fd, msg, strlen(msg));
                }
                if(removeKey(cache, doc.key, ind, indexed_files) == -1)
                {
                    char msg[BUFFER_SIZE];
                    snprintf(msg, sizeof(msg), "Erro ao remover o documento (key=%d)\n", doc.key);
                    write(server_fd, msg, strlen(msg));
                }
                else
                {
                    char msg[BUFFER_SIZE];
                    snprintf(msg, sizeof(msg), "Documento %d removido\n", doc.key);
                    write(server_fd, msg, strlen(msg));
                }

                exit(0);
            }
            else
            {
                close(client_fd);
                printf("Remoção em PID = %d\n", pid);
            }
        }
        // else if (strcmp(doc.flag, "-l") == 0)
        //{
        //     pid_t pid = fork();
        //     if (pid == 0)
        //     {
        //         int nr = searchKeyWords();
        //         if (nr >= 0)
        //         {
        //             char msg[BUFFER_SIZE];
        //             snprintf(msg, sizeof(msg), "A palavra %s está no ficheiro %d vezes\n", doc.palavra, nr);
        //             ssize_t bytes_written = write(server_fd, msg, strlen(msg));
        //             if (bytes_written == -1)
        //             {
        //                 perror("Erro ao escrever no pipe do servidor");
        //             }
        //         }
        //         else
        //         {
        //             const char *msg = "Arquivo não indexado\n";
        //             ssize_t bytes_written = write(server_fd, msg, strlen(msg));
        //             if (bytes_written == -1)
        //             {
        //                 perror("Erro ao escrever no pipe do servidor");
        //             }
        //         }
        //     }
        //     else
        //     {
        //         close(client_fd);
        //         printf("Contagem em PID = %d\n", pid);
        //     }
        // }
        //  Fechar os file descriptors no final de cada iteração
        close(client_fd);
        close(server_fd);
    }

    // Código nunca deve chegar aqui, mas por precaução
    unlink(SERVER_PIPE);
    unlink(CLIENT_PIPE);
    return EXIT_SUCCESS;
}