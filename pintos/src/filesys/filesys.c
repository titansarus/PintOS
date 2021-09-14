#include "filesys/filesys.h"
#include "filesys/cache.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format)
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  cache_init ();
  free_map_init ();

  if (format)
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void)
{
  free_map_close ();
  cache_flush (fs_device);
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size, bool is_dir)
{
  block_sector_t inode_sector = 0;

  char directory[strlen (name) + 1];
  char file_name[NAME_MAX + 1];
  directory[0] = file_name[0] = '\0';

  bool path_splitted = split_path (name, directory, file_name);
  struct dir *dir = dir_open_path (directory);

  bool success = (dir != NULL
                  && path_splitted
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size, is_dir)
                  && dir_add (dir, file_name, inode_sector, is_dir));
  if (!success && inode_sector != 0)
    free_map_release (inode_sector, 1);
  dir_close (dir);

  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  char directory[strlen (name) + 1];
  char file_name[NAME_MAX + 1];
  directory[0] = '\0';
  file_name[0] = '\0';

  bool path_splitted = split_path (name, directory, file_name);
  struct dir *dir = dir_open_path (directory);

  struct inode *inode = NULL;
  if (dir == NULL || !path_splitted)
    return NULL;

  if (strlen (file_name) == 0)
    inode = dir_get_inode (dir);
  else
    {
      dir_lookup (dir, file_name, &inode);
      dir_close (dir);
    }

  if (inode == NULL || inode_is_removed (inode))
    return NULL;

  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name)
{
  char directory[strlen (name) + 1];
  char file_name[NAME_MAX + 1];
  directory[0] = '\0';
  file_name[0] = '\0';

  bool path_splitted = split_path (name, directory, file_name);
  struct dir *dir = dir_open_path (directory);

  bool success = path_splitted && (dir != NULL) && dir_remove (dir, file_name);
  dir_close (dir);

  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}
