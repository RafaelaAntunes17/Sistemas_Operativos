#define SERVER "dserver"
#define CLIENT "dclient"

#define MAX_DOCUMENTS 100
#define MAXBUFFER 1024
#define MAX_CACHE_SIZE 1024

typedef struct

{
    char title[200];
    char authors[200];
    char year[5];
    char path[64];
} Document;

typedef struct
{
    Document documents[MAX_DOCUMENTS];
    int count;
} DocumentList;
