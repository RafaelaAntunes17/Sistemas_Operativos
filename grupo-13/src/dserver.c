#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv){
    if (argc != 3){
        write(1,"./dserver document_folder cache_size\n", 38);
        return -1;     
    }
    char *document_folder = argv[1];
    int cache_size = atoi(argv[2]);
    if (cache_size <= 0){
        write(1,"cache_size must be a positive integer\n", 38);
        return -1;
    }
    if (chdir(document_folder) == -1){
        write(1,"document_folder must be a valid path\n", 38);
        return -1;
    }

    

}