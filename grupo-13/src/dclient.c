#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ./dclient <command> [options]\n");
        return 1;
    }

    if (strcmp(argv[1], "-a") == 0) {
        if (argc != 6) {
            fprintf(stderr, "Usage: ./dclient -a <title> <authors> <year> <path>\n");
            return 1;
        }
        indexação();
    } else if (strcmp(argv[1], "-c") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: ./dclient -c <document_id>\n");
            return 1;
        }
        consulta();
    } else if (strcmp(argv[1], "-d") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: ./dclient -d <document_id>\n");
            return 1;
        }
        remover();
    } else if (strcmp(argv[1], "-l") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage: ./dclient -l <document_id> <keyword>\n");
            return 1;
        }
        src_palavra_num();
    } else if (strcmp(argv[1], "-s") == 0) {
        if (argc != 3 && argc != 4) {
            fprintf(stderr, "Usage: ./dclient -s <keyword> [process_count]\n");
            return 1;
        }
        src_palavra_listid();
    } else if (strcmp(argv[1], "-f") == 0) {
        if (argc != 2) {
            fprintf(stderr, "Usage: ./dclient -f\n");
            return 1;
        }
        servidor();
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        return 1;
    }

    return 0;
}