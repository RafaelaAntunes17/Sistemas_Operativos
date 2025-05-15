#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

void append_to_file(int key, const char *title, const char *authors, const char *year, const char *path);
int create_key();
int check_existing_document(const char *title, const char *authors, const char *year, const char *path);
char *searchKey(int key);
int removeKey(Meta cache, int key, int ind, int indexed_files);
int fileToCache(Meta cache, int cache_size);
void print_ocupados(Meta tabela);
int indexMeta(Meta cache, char *title, char *authors, char *year, char *path, int key);
int apagaMeta(Meta cache, int key);