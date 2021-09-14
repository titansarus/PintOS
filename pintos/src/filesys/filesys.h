#ifndef FILESYS_FILESYS_H
#define FILESYS_FILESYS_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "threads/synch.h"

/* Sectors of system file inodes. */
#define FREE_MAP_SECTOR 0       /* Free map file inode sector. */
#define ROOT_DIR_SECTOR 1       /* Root directory file inode sector. */

/* reserved file ids */
#define STDIN_FILENO  0
#define STDOUT_FILENO 1


/* Block device that contains the file system. */
struct block *fs_device;
struct lock fs_lock;

void filesys_init (bool format);
void filesys_done (void);
bool filesys_create (const char *name, off_t initial_size);
struct file *filesys_open (const char *name);
bool filesys_remove (const char *name);

/* thread safe abstraction over functions above (using fs_lock) */
bool filesys_create_l (const char *, off_t);
struct file * filesys_open_l (const char *);
bool filesys_remove_l (const char *);

#endif /* filesys/filesys.h */
