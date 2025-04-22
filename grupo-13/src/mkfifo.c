#include <stdio.h>

int main(int argc, char **argv)
{
    int res = mkfifo("client-to-server", 0666);
    if (res == -1)
    {
        perror("Error creating FIFO");
        return -1;
    }
    res = mkfifo("server-to-client", 0666);
    if (res == -1)
    {
        perror("Error creating FIFO");
        return -1;
    }
    printf("FIFOs created successfully.\n");

}