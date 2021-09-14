#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include <stdio.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "filesys/cache.h"
#include "threads/synch.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

/* End-of-File */
#define EOF (size_t)-1

/* Block Sector Counts */
#define DIRECT_BLK_CNT 123
#define INDIRECT_BLK_CNT 128

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    block_sector_t db[DIRECT_BLK_CNT];
    block_sector_t ib;
    block_sector_t dib;

    bool is_dir;                        /* Indicator of directory file */
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
  };

/* indirect-block structure */
struct indirect_blk_sector
  {
    block_sector_t blks[INDIRECT_BLK_CNT];
  };

typedef struct indirect_blk_sector indirect_blk_sector_t;

/* Min function */
#define min(a,b) ((a)<(b))?(a):(b)

/* SHAKH */
#define INODE_DISK_PROPERTY(TYPE, MEMBER, INODE)          \
  ASSERT (INODE != NULL);                                 \
  struct inode_disk *disk_inode = get_inode_disk (INODE); \
  TYPE MEMBER = disk_inode->MEMBER;                       \
  free(disk_inode)

/* In-memory inode. */
struct inode
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct lock i_lock;                 /* Synchronization. */
  };

/* Allocation Functions */
static bool inode_allocate (struct inode_disk *, off_t);
static bool inode_allocate_sector (block_sector_t *);
static bool inode_allocate_direct (struct inode_disk *, size_t);
static bool inode_allocate_indirect (block_sector_t *, size_t);
static bool inode_allocate_dbl_indirect (block_sector_t *, size_t);

/* Deallocation Functions */
static void inode_deallocate_direct(struct inode_disk *, size_t);
static void inode_deallocate (struct inode *);
static void inode_deallocate_indirect (block_sector_t , size_t);
static void inode_deallocate_dbl_indirect (block_sector_t , size_t);



/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos)
{
  ASSERT (inode != NULL);
  block_sector_t result = -1;
  struct inode_disk *disk_inode = get_inode_disk (inode);

  if (pos >= disk_inode->length)
      goto b2s_end;

  off_t target_blk = pos / BLOCK_SECTOR_SIZE;

  /* direct block */
  if (target_blk < DIRECT_BLK_CNT)
    {
      result = disk_inode->db[target_blk];
      goto b2s_end;
    }

  /* indirect block */
  if (target_blk < DIRECT_BLK_CNT + INDIRECT_BLK_CNT)
    {
      target_blk -= DIRECT_BLK_CNT;

      indirect_blk_sector_t ib;
      cache_read (fs_device, disk_inode->ib,
                  &ib, 0, BLOCK_SECTOR_SIZE);
      result = ib.blks[target_blk];
      goto b2s_end;
    }

  /* double indirect block */
  target_blk -= (DIRECT_BLK_CNT + INDIRECT_BLK_CNT);
  indirect_blk_sector_t ib;

  /* get double indirect and indirect block index */
  int did_target_blk = target_blk / INDIRECT_BLK_CNT;
  int id_target_blk  = target_blk % INDIRECT_BLK_CNT;

  /* Read double indirect block, then indirect block */
  cache_read (fs_device, disk_inode->dib, &ib, 0, BLOCK_SECTOR_SIZE);
  cache_read (fs_device, ib.blks[did_target_blk], &ib, 0, BLOCK_SECTOR_SIZE);
  result = ib.blks[id_target_blk];

b2s_end:
  free(disk_inode);
  return result;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void)
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false, is_dir = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      disk_inode->is_dir = is_dir;
      disk_inode->length = length;
      disk_inode->magic  = INODE_MAGIC;
      if (inode_allocate (disk_inode, length))
        {
          cache_write (fs_device, sector, disk_inode, 0, BLOCK_SECTOR_SIZE);
          success = true;
        }
      free (disk_inode);
    }
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (struct list_elem *e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e))
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector)
        {
          inode_reopen (inode);
          return inode;
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  lock_init (&inode->i_lock);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode)
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);

      /* Deallocate blocks if removed. */
      if (inode->removed)
        {
          free_map_release (inode->sector, 1);
          inode_deallocate (inode);
        }

      free (inode);
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode)
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset)
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;

  while (size > 0)
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = min(size , min(inode_left , sector_left));
      if (chunk_size <= 0)
        break;

      cache_read (fs_device, sector_idx, (void *)(buffer + bytes_read),
                  sector_ofs, chunk_size);

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)
{
  const uint8_t *buffer = buffer_;

  if (inode->deny_write_cnt)
    return 0;

  /* extend file if needed */
  if (byte_to_sector (inode, offset + size - 1) == EOF)
    {
      struct inode_disk *disk_inode = get_inode_disk (inode);

      if (!inode_allocate (disk_inode, offset + size))
        {
          free (disk_inode);
          return 0;
        }

      disk_inode->length = offset + size;
      cache_write (fs_device, inode_get_inumber (inode), (void *)disk_inode, 0, BLOCK_SECTOR_SIZE);
      free (disk_inode);
    }

  off_t bytes_written = 0;
  
  while (size > 0)
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = min (size, min (inode_left, sector_left));
      if (chunk_size <= 0)
        break;

      cache_write (fs_device, sector_idx, (void *)(buffer + bytes_written),
                   sector_ofs, chunk_size);

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }

  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode)
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode)
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

struct inode_disk *
get_inode_disk (const struct inode *inode)
{
  ASSERT (inode != NULL);
  struct inode_disk *disk_inode = malloc (sizeof *disk_inode);
  cache_read (fs_device, inode_get_inumber (inode), (void *)disk_inode, 0, BLOCK_SECTOR_SIZE);
  return disk_inode;
}

off_t
inode_length (const struct inode *inode)
{
  INODE_DISK_PROPERTY(off_t, length, inode);
  return length;
}

bool
inode_is_dir (const struct inode *inode)
{
  INODE_DISK_PROPERTY(bool, is_dir, inode);
  return is_dir;
}

bool
inode_is_removed (const struct inode *inode)
{
  ASSERT (inode != NULL);
  return inode->removed;
}

/* SHAKH */
#define TRY_ALLOCATE(MAX_BLK_CNT, ALLOCATE)   \
  blk_cnt = min (num_sectors, MAX_BLK_CNT);   \
  if (!ALLOCATE) return false;                \
  num_sectors -= blk_cnt;                     \
  if (num_sectors == 0) return true
  

static bool
inode_allocate (struct inode_disk *disk_inode, off_t length)
{
  ASSERT (disk_inode != NULL);
  if (length < 0) return false;

  size_t blk_cnt, num_sectors = bytes_to_sectors (length);

  /* Allocate direct Blocks */
  TRY_ALLOCATE (DIRECT_BLK_CNT,
                inode_allocate_direct (disk_inode, blk_cnt));

  /* Allocate indirect Block */  
  TRY_ALLOCATE (INDIRECT_BLK_CNT,
                inode_allocate_indirect (&disk_inode->ib, blk_cnt));

  /* Allocate double-indirect Block */
  TRY_ALLOCATE (INDIRECT_BLK_CNT * INDIRECT_BLK_CNT,
                inode_allocate_dbl_indirect(&disk_inode->dib, blk_cnt));

  /* Shouldn't go past this point */
  ASSERT (num_sectors == 0);
  return false;
}

static bool
inode_allocate_direct (struct inode_disk *disk_inode, size_t cnt)
{
  for (size_t i = 0; i < cnt; i++)
    if (!inode_allocate_sector (&disk_inode->db[i]))
      return false;
  return true;
}

static bool
inode_allocate_sector (block_sector_t *sector_num)
{
  static char buffer[BLOCK_SECTOR_SIZE];
  if (!*sector_num)
    {
      if (!free_map_allocate (1, sector_num))
        return false;
      cache_write (fs_device, *sector_num, buffer, 0, BLOCK_SECTOR_SIZE);
    }
  return true;
}

static bool
inode_allocate_indirect (block_sector_t *sector_num, size_t cnt)
{
  /* Allocate if indirect block sector hasn't been allocated. */
  if (!inode_allocate_sector (sector_num))
    return false;

  indirect_blk_sector_t ib;
  cache_read (fs_device, *sector_num, &ib, 0, BLOCK_SECTOR_SIZE);

  for (size_t i = 0; i < cnt; i++)
    if (!inode_allocate_sector (&ib.blks[i]))
      return false;

  cache_write (fs_device, *sector_num, &ib, 0, BLOCK_SECTOR_SIZE);

  return true;
}

static bool
inode_allocate_dbl_indirect (block_sector_t *sector_num, size_t cnt)
{
  /* Allocate if double indirect block sector hasn't been allocated. */
  if (!inode_allocate_sector (sector_num))
    return false;

  indirect_blk_sector_t ib;
  cache_read (fs_device, *sector_num, &ib, 0, BLOCK_SECTOR_SIZE);

  size_t num_sectors, blk_cnt = DIV_ROUND_UP (cnt, INDIRECT_BLK_CNT);
  for (size_t i = 0; i < blk_cnt; i++)
    {
      num_sectors = min (cnt, INDIRECT_BLK_CNT);
      if (!inode_allocate_indirect (&ib.blks[i], num_sectors))
        return false;
      cnt -= num_sectors;
    }

  cache_write (fs_device, *sector_num, &ib, 0, BLOCK_SECTOR_SIZE);

  return true;
}

/* SHAKH */
#define TRY_DEALLOCATE(MAX_BLK_CNT, DEALLOCATE)   \
  blk_cnt = min (num_sectors, MAX_BLK_CNT);       \
  DEALLOCATE;                                     \
  num_sectors -= blk_cnt;                         \
  if (num_sectors == 0)                           \
    {                                             \
      free (disk_inode);                          \
      return;                                     \
    }

static void
inode_deallocate (struct inode *inode)
{
  ASSERT (inode != NULL);

  struct inode_disk *disk_inode = get_inode_disk (inode);

  off_t length = disk_inode->length;
  if (length < 0) return;

  size_t blk_cnt, num_sectors = bytes_to_sectors (length);

  /* Deallocate direct blocks */
  TRY_DEALLOCATE (DIRECT_BLK_CNT,
                  inode_deallocate_direct(disk_inode, blk_cnt));

  /* Deallocate indirect block */
  TRY_DEALLOCATE (INDIRECT_BLK_CNT,
                  inode_deallocate_indirect (disk_inode->ib, blk_cnt));

  /* Deallocate double indirect block */
  TRY_DEALLOCATE (INDIRECT_BLK_CNT * INDIRECT_BLK_CNT,
                  inode_deallocate_dbl_indirect (disk_inode->dib, blk_cnt));

  NOT_REACHED();
}

static void
inode_deallocate_direct(struct inode_disk *disk_inode, size_t cnt){
  for (size_t i = 0; i < cnt; i++)
    free_map_release (disk_inode->db[i], 1);
}

static void
inode_deallocate_indirect (block_sector_t sector_num, size_t cnt)
{
  indirect_blk_sector_t ib;
  cache_read (fs_device, sector_num, &ib, 0, BLOCK_SECTOR_SIZE);

  for (size_t i = 0; i < cnt; i++)
    free_map_release (ib.blks[i], 1);

  free_map_release (sector_num, 1);
}

static void
inode_deallocate_dbl_indirect (block_sector_t sector_num, size_t cnt)
{
  indirect_blk_sector_t ib;
  cache_read (fs_device, sector_num, &ib, 0, BLOCK_SECTOR_SIZE);

  size_t num_sectors, blk_cnt = DIV_ROUND_UP (cnt, INDIRECT_BLK_CNT);
  for (size_t i = 0; i < blk_cnt; i++)
    {
      num_sectors = min (cnt, INDIRECT_BLK_CNT);
      inode_deallocate_indirect (ib.blks[i], num_sectors);
      cnt -= num_sectors;
    }

  free_map_release (sector_num, 1);
}
