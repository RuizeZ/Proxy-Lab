#include "cache.h"

int blockusedtime = 0, mintime = 0;
cache_block* cacheaddr;
int maxobject;
//intialize cache with malloc
void init_cache()
{
    cacheaddr = (cache_block *)malloc(MAX_CACHE_SIZE);
    maxobject = MAX_CACHE_SIZE/sizeof(cache_block);
    printf("maxobject: %d\n", maxobject);
}

//if miss cache return 0, hit cache write content to fd
int read_cache(char* url, int fd)
{
    cache_block *currentBlock, *blocktoReplace;   
    for (int i = 0; i < maxobject; i++)
    {
        currentBlock = cacheaddr + (i * sizeof(cache_block));
        if (i == 0 || (currentBlock->time <= mintime && currentBlock->time != 0))
        {
            mintime = currentBlock->time;
            blocktoReplace = currentBlock;
        }
        
        if(!strcmp(currentBlock->url, url))
        {
            blockusedtime++;
            currentBlock->time = blockusedtime;
            Rio_writen(fd, currentBlock->data, currentBlock->datasize);
            break;
        }  
         
    }


}
//save value to cache
void write_cache(char* url, char* data, int len)
{

}
//free cache
void free_cache();