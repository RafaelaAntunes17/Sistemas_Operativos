#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

void append_to_metadata(int key, const char *title, const char *authors, const char *year, const char *path);
int create_key();
int check_existing_document(const char *title, const char *authors, const char *year, const char *path);
char *searchKey(int key);