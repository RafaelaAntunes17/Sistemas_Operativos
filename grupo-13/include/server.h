#include <limits.h>
#define SERVER_PIPE "server_to_client"
#define CLIENT_PIPE "client_to_server"
#define METADATA_FILE "docserver_metadata.txt"
#define BUFFER_SIZE 1024
#define MAX_TITLE 200
#define MAX_AUTHORS 200
#define MAX_YEAR 5
#define MAX_PATH 64

extern int global_key;

typedef struct
{
    char flag[3];

    // Indexação
    char title[MAX_TITLE];
    char authors[MAX_AUTHORS];
    char year[MAX_YEAR];
    char path[MAX_PATH];

    int key;

    char *palavra;
    int nr_procuras;
} DocumentMetadata;

typedef struct 
{
    int contador;
    int key;

    char title[MAX_TITLE];
    char authors[MAX_AUTHORS];
    char year[MAX_YEAR];
    char path[MAX_PATH];

    struct archive_metadata *next;

} ArchiveMetadata;

