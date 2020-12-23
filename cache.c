#include "cache.h"

int blockusedtime = 0, mintime = 0;
cache_block* cacheaddr, *blocktoReplace;
int maxobject;
//intialize cache with malloc
void init_cache()
{
    cacheaddr = (cache_block *)malloc(MAX_CACHE_SIZE);
    maxobject = MAX_CACHE_SIZE/sizeof(cache_block);
}

//if miss cache return 0, hit cache write content to fd
int read_cache(char* url, int fd)
{
    cache_block *currentBlock;
    printf("start to read cache\n");
    for (int i = 0; i < maxobject; i++)
    {
        currentBlock = cacheaddr + i;
        if (i == 0 || (currentBlock->time < mintime))
        {
            mintime = currentBlock->time;
            blocktoReplace = currentBlock;
        }
        if(!strcmp(currentBlock->url, url))
        {
            blockusedtime++;
            currentBlock->time = blockusedtime;
            Rio_writen(fd, currentBlock->data, currentBlock->datasize);
            return 1;
        }
    }
    return 0;
}
//save value to cache
void write_cache(char* url, char* data, int len)
{
    strcpy(blocktoReplace->url, url);
    strcpy(blocktoReplace->data, data);
    blocktoReplace->datasize = len;
    blockusedtime++;
    blocktoReplace->time = blockusedtime;
    return;
}
//free cache
void free_cache(){
    free(cacheaddr);
}