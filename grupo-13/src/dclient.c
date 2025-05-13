#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "../include/server.h"

int main(int argc, char **argv)
{
    char curr_flag;
    if (argc < 2)
    {
        perror("Error: No command provided");
        return 1;
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    if (strcmp(argv[1], "-f") == 0)
    {
        if (argc != 2)
        {
            perror("Error: Invalid number of arguments for -f");
            return 1;
        }
        snprintf(buffer, BUFFER_SIZE, "-f");
        curr_flag = "f";
    }
    else if (strcmp(argv[1], "-a") == 0)
    {
        if (argc != 5)
        {
            perror("Error: Invalid number of arguments for -a");
            return 1;
        }
        snprintf(buffer, BUFFER_SIZE, "-a|%s|%s|%s|%s", argv[2], argv[3], argv[4], argv[5]);
        curr_flag = "a";
    }
    else if (strcmp(argv[1], "-d") == 0)
    {
        if (argc != 3)
        {
            perror("Error: Invalid number of arguments for -d");
            return 1;
        }
        snprintf(buffer, BUFFER_SIZE, "-d|%s", argv[2]);
        curr_flag = "d";
    }
    else if (strcmp(argv[1], "-c") == 0)
    {
        if (argc != 3)
        {
            perror("Error: Invalid number of arguments for -d");
            return 1;
        }
        snprintf(buffer, BUFFER_SIZE, "-c|%s", argv[2], argv[3]);
        curr_flag = "c";
    }
    else if (strcmp(argv[1], "-l") == 0)
    {
        if (argc != 4)
        {
            perror("Error: Invalid number of arguments for -l");
            return 1;
        }
        snprintf(buffer, BUFFER_SIZE, "-l|%s|%s", argv[2], argv[3]);
        curr_flag = "l";
    }
    else if (strcmp(argv[1], "-s") == 0)
    {
        if (argc != 3)
        {
            perror("Error: Invalid number of arguments for -s");
            return 1;
        }
        snprintf(buffer, BUFFER_SIZE, "-s|%s|%s", argv[2], argv[3]);
        curr_flag = "s";
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

    ssize_t bytes_written = write(write_fd, buffer, strlen(buffer) + 1);
    if (bytes_written == -1)
    {
        perror("Error writing to client pipe");
        close(write_fd);
        return 1;
    }
    printf("Command sent to server: -%s\n", curr_flag);

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
