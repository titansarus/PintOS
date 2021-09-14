#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include "off_t.h"
#include "devices/block.h"
#include "threads/synch.h"
#include "lib/kernel/list.h"

#define BUFFER_CACHE_SIZE 64

struct cache_blk_status {
    int read_cnt;
    int write_cnt;
};

struct cache_block {
    block_sector_t sector;
    char data[BLOCK_SECTOR_SIZE];
    int dirty;
    int valid;
    struct lock c_lock;
    struct cache_blk_status status;

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
int LRU_hit;
int LRU_miss;

/* Initialize cache. */
void cache_init (void);

/* Read chunk_size bytes of data from cache starting from sector_index at position offest,
   into destination. */
void cache_read (struct block *, block_sector_t , void *, off_t , int);

/* Write chunk_size bytes of data into cache starting from sector_index at position offest,
   from source. */
void cache_write (struct block *, block_sector_t , void *, off_t , int);

/* Write-back all cache blocks and invalidate them.*/
void cache_flush(struct block *);

/* cache stats API for SYS_CACHE_SPEC syscall */
uint32_t cache_get_total_read_cnt (void);
uint32_t cache_get_total_write_cnt(void);
uint32_t cache_get_total_miss(void);
uint32_t cache_get_total_hit (void);

#endif

