#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_BUFF 1024

void status(char pid[])
{
    int pipe_pid = open(pid, O_RDONLY, 0666);
    if (pipe_pid == -1)
    {
        perror("Erro a abrir o pipe");
        return;
    }

    int i = 0;
    char buffer;

    while (read(pipe_pid, &buffer, 1) > 0)
    {
        write(1, &buffer, 1);
    }
    close(pipe_pid);
    unlink(pid);
    exit(0);
}

int sizeFile(char line[])
{
    int fd = open(line, O_RDONLY);
    if (fd == -1)
    {
        perror("Erro a abrir o ficheiro");
        return -1;
    }
    int size = lseek(fd, 0, SEEK_END);
    close(fd);
    return size;
}

int main(int argc, char **argv)
{
    char buffer[MAX_BUFF];

    if (argc == 1)
    {
        int size = snprintf(buffer, MAX_BUFF, "Usage: %s <document_folder> <cache_size>\n", argv[0]);
        write(1, buffer, size);
    }
    else
    {
        char *document_folder = argv[1];
        int cache_size = atoi(argv[2]);

        if (cache_size <= 0)
        {
            perror("Cache size must be a positive integer");
            return -1;
        }

        if (mkfifo("dserver_fifo", 0666) == -1)
        {
            perror("Error creating FIFO_server");
            return -1;
        }
        
        int fd = open("dserver_fifo", O_RDONLY);
        if (fd == -1)
        {
            perror("Error opening FIFO_server");
            return -1;
        }

        int fd_cl = open("dclient_fifo", O_WRONLY);
        if (fd_cl == -1)
        {
            perror("Error opening FIFO_client");
            close(fd);
            return -1;
        }

        ssize_t bytes_read;
        printf("Reading from FIFO...\n");
        while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
        {
            if (write(fd_cl, buffer, bytes_read) == -1)
            {
                perror("Error writing to FIFO");
                close(fd);
                close(fd_cl);
                return -1;
            }
        }

        char fifo_client_name[20];
        snprintf(fifo_client_name, sizeof(fifo_client_name), "dclient_fifo_%d", getpid());
        if(unlink(fifo_client_name) == -1)
        {
            perror("Error unlinking FIFO");
            return -1;
        }
        int fifo_client = open(fifo_client_name, O_WRONLY, 0666);
        snprintf(fifo_client_name, sizeof(fifo_client_name), "dclient_fifo_%d", getpid());
        write(fifo_client_name, buffer, sizeof(buffer));
        
        close(fifo_client);
        close(fd);
        close(fd_cl);
    }
    return 0;
}