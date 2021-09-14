#include <debug.h>
#include <string.h>

#include "cache.h"
#include "threads/synch.h"
#include "lib/kernel/list.h"
#include "devices/block.h"


#define min(a,b) ((a)<(b))?(a):(b)

void cache_block_init(struct cache_block* c_b){
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
    
    lock_release(&LRU_modify_lock);
}

void cache_write_back(struct block *fs_device, struct cache_block* cb)
{
    ASSERT(lock_held_by_current_thread(&cb->c_lock));
    
    block_write(fs_device,cb->sector,cb->data);
    cb->dirty=0;
}


struct cache_block *get_cache_block (struct block *fs_device, block_sector_t sector)
{
    // if block is already loaded in the cache buffer
    for (int i=0; i < BUFFER_CACHE_SIZE; i++){
        if(cache_blocks[i].valid && cache_blocks[i].sector==sector){
            lock_acquire(&LRU_modify_lock);
            
            // LRU: update the least recently used (front of the queue) cache_block 
            list_remove(&cache_blocks[i].elem);
            list_push_back(&LRU,&cache_blocks[i].elem);
            
            lock_release(&LRU_modify_lock);
            return &cache_blocks[i];
        }
    }

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
    
    lock_release(&old_cb->c_lock);

    // LRU: 2. push the new cache_block in cache 
    list_push_back(&LRU,&old_cb->elem);
    lock_release(&LRU_modify_lock);
    return old_cb;
}

void cache_read (struct block *fs_device, block_sector_t sector, void *dst, off_t offset, int chunk_size)
{
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