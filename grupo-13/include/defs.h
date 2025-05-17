#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

void append_to_file(int key, const char *title, const char *authors, const char *year, const char *path);
int create_key();
int check_existing_document(const char *title, const char *authors, const char *year, const char *path);
char *searchKey(int key);
int removeKey(int key);
int fileToCache(Meta cache, int cache_size);
int indexMeta(Meta cache, char *title, char *authors, char *year, char *path, int key);
int apagaMeta(Meta cache, int key);
int findKey(char *path, char *palavra);
int searchKeyWords(Meta cache, int key, char *palavra, char *palavra2);
int find_lowest_available_key();
void update_access_time(Meta cache, int key);
int find_lru_entry(Meta cache, int cache_size);
int findKey(char *path, char *palavra);
