#include <debug.h>
#include <string.h>

#include "cache.h"
#include "threads/synch.h"
#include "lib/kernel/list.h"
#include "devices/block.h"


#define min(a,b) ((a)<(b))?(a):(b)

static void cache_block_init(struct cache_block* c_b){
    lock_init(&c_b->c_lock);
    c_b->dirty=0;
    c_b->valid=0;
}

void cache_init(){
    lock_init(&LRU_modify_lock);
    
    lock_acquire(&LRU_modify_lock);
    list_init(&LRU);

    for (int i=0;i<BUFFER_CACHE_SIZE;i++){
        cache_block_init(cache_blocks + i);
        list_push_back (&LRU, &(cache_blocks[i].elem));
    }
    
    LRU_miss=0;
    LRU_hit=0;
    LRU_write=0;
    lock_release(&LRU_modify_lock);
}

static void cache_write_back(struct block *fs_device, struct cache_block* cb)
{
    ASSERT(lock_held_by_current_thread(&cb->c_lock));
    
    block_write(fs_device,cb->sector,cb->data);
    cb->dirty=0;
    LRU_write++;
}


static struct cache_block *get_cache_block (struct block *fs_device, block_sector_t sector)
{
    // if block is already loaded in the cache buffer
    for (int i=0; i < BUFFER_CACHE_SIZE; i++){
        if(cache_blocks[i].valid && cache_blocks[i].sector==sector){
            lock_acquire(&LRU_modify_lock);
            
            // LRU: update the least recently used (front of the queue) cache_block 
            list_remove(&cache_blocks[i].elem);
            list_push_back(&LRU,&cache_blocks[i].elem);
            
            lock_release(&LRU_modify_lock);

            // cache hit
            LRU_hit++;
            return &cache_blocks[i];
        }
    }
    
    // cache missed
    LRU_miss++;

    // fetch the block and store it in the cache
    lock_acquire(&LRU_modify_lock);
    // LRU: 1. get the least recently used (front of the queue) cache_block 
    struct cache_block* old_cb = list_entry(list_pop_front (&LRU),struct cache_block, elem);
    lock_acquire(&(old_cb->c_lock));
    
    // write back if dirty
    if (old_cb->valid && old_cb->dirty)
            cache_write_back(fs_device,old_cb);
    
    
    // update new cache block
    block_read(fs_device, sector, old_cb->data);
    old_cb->valid=1;
    old_cb->dirty=0;
    old_cb->sector=sector;
    
    lock_release(&old_cb->c_lock);

    // LRU: 2. push the new cache_block in cache 
    list_push_back(&LRU,&old_cb->elem);
    lock_release(&LRU_modify_lock);
    return old_cb;
}

void cache_read (struct block *fs_device, block_sector_t sector, void *dst, off_t offset, int chunk_size)
{
    ASSERT(offset<BLOCK_SECTOR_SIZE);

    int bytes_left = chunk_size;
    while (bytes_left > 0)
    {
        struct cache_block *cache = get_cache_block(fs_device,sector);
        lock_acquire(&cache->c_lock);

        memcpy(dst, &(cache->data[offset]), min(bytes_left, BLOCK_SECTOR_SIZE));

        lock_release(&cache->c_lock);
        
        sector++;
        dst += BLOCK_SECTOR_SIZE;
        bytes_left -= BLOCK_SECTOR_SIZE;
        offset = 0;
    }
}

void cache_write (struct block *fs_device, block_sector_t sector, void *src, off_t offset, int chunk_size)
{
    ASSERT(offset<BLOCK_SECTOR_SIZE);
    
    int bytes_left = chunk_size;
    while (bytes_left > 0)
    {
        struct cache_block *cache = get_cache_block(fs_device, sector);

        lock_acquire(&cache->c_lock);
        
        memcpy(&(cache->data[offset]), src, min(bytes_left, BLOCK_SECTOR_SIZE));
        cache->dirty=1;

        lock_release(&cache->c_lock);

        sector++;
        src += BLOCK_SECTOR_SIZE;
        bytes_left -= BLOCK_SECTOR_SIZE;
        offset = 0;
    }
} 

void cache_flush(struct block *fs_device)
{
    for (int i=0;i<BUFFER_CACHE_SIZE;i++)
    {   
        struct cache_block* cb = cache_blocks + i;
        if(!(cb->valid && cb->dirty))continue;

        lock_acquire(&cb->c_lock);
        cache_write_back(fs_device,cb);
        lock_release(&cb->c_lock);
    }
}

void cache_invalidate(struct block *fs_device){
    cache_flush(fs_device);
    for (int i=0;i<BUFFER_CACHE_SIZE;i++){
        lock_acquire(&cache_blocks[i].c_lock);
        cache_blocks[i].valid=0;
        lock_release(&cache_blocks[i].c_lock);
    }
}


uint32_t 
cache_get_total_read_cnt (){
    return LRU_miss;
}

uint32_t
cache_get_total_write_cnt(){
    return LRU_write;
}

uint32_t 
cache_get_total_miss (){
    return LRU_miss;
}

uint32_t 
cache_get_total_hit (){
    return LRU_hit;
}
