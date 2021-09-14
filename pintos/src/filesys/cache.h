#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include "off_t.h"
#include "devices/block.h"
#include "threads/synch.h"
#include "lib/kernel/list.h"

#define BUFFER_CACHE_SIZE 64

struct cache_block {
    block_sector_t sector;
    char data[BLOCK_SECTOR_SIZE];
    int dirty;
    int valid;
    struct lock c_lock;

#ifdef SHAKH
    struct condition ok_to_read;
    struct condition ok_to_write;
    int waiting_w;
    int waiting_r;
    int available_w;
    int available_r;
#endif

    struct list_elem elem;
};

struct cache_block cache_blocks[BUFFER_CACHE_SIZE];
struct list LRU;
struct lock LRU_modify_lock;

/* Initialize cache. */
void cache_init (void);

/* Read chunk_size bytes of data from cache starting from sector_index at position offest,
   into destination. */
void cache_read (struct block *fs_device, block_sector_t sector, void *dst, off_t offset, int chunk_size);

/* Write chunk_size bytes of data into cache starting from sector_index at position offest,
   from source. */
void cache_write (struct block *fs_device, block_sector_t sector, void *src, off_t offset, int chunk_size);


// /* Write entire cache to disk. */
// void cache_flush (struct block *fs_device);

#endif

