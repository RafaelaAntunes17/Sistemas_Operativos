#include <limits.h>
#define SERVER_PIPE "/tmp/dserver_pipe"
#define CLIENT_PIPE "/tmp/dclient_pipe"
#define METADATA_FILE "docserver_metadata.dat"
#define BUFFER_SIZE 1024
#define MAX_TITLE 200
#define MAX_AUTHORS 200
#define MAX_YEAR 5
#define MAX_PATH 64



typedef struct {
    int key;
    char title[MAX_TITLE];
    char authors[MAX_AUTHORS];
    char year[MAX_YEAR];
    char path[MAX_PATH];
} DocumentMetadata;

typedef struct {
    DocumentMetadata *items;
    int capacity;
    int size;
    int *access_times;
    int current_time;
} Cache;

typedef struct {
    DocumentMetadata *documents;
    int count;
    int next_key;
    char document_folder[BUFFER_SIZE];
    Cache cache;
} ServerState;

// Funções do servidor
void init_server(ServerState *state, const char *document_folder, int cache_size);
void handle_command(ServerState *state, const char *command);
void free_resources(ServerState *state);

// Funções de documentos
int add_document(ServerState *state, DocumentMetadata doc);
int remove_document(ServerState *state, int key);
DocumentMetadata* get_document(ServerState *state, int key);

// Funções de cache
void init_cache(Cache *cache, int capacity);
void free_cache(Cache *cache);
DocumentMetadata* get_from_cache(Cache *cache, int key);
void add_to_cache(Cache *cache, DocumentMetadata doc);

// Funções de persistência
void save_metadata(ServerState *state);
void load_metadata(ServerState *state);

// Funções de pesquisa
int count_lines_with_keyword(int key, const char *keyword);
int* search_documents_with_keyword(const char *keyword, int *result_count);

