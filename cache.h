#include "csapp.h"
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
typedef struct cache_block{
    int time;
    char url[MAXLINE];
    int datasize;
    char data[MAX_OBJECT_SIZE];
} cache_block;

//intialize cache with malloc
void init_cache();
//if miss cache return 0, hit cache write content to fd
int read_cache(char* url, int fd);
//save value to cache
void write_cache(char* url, char* data, int len);
//free cache
void free_cache();