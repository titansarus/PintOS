#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/directory.h"
#ifdef USERPROG
#include "userprog/exception.h"
#include "userprog/process.h"
#endif

#define STDIN_FILENO  0
#define STDOUT_FILENO 1

static void syscall_handler (struct intr_frame *);

static struct lock fs_lock;

/* Validate arguments for all syscalls */
static bool
validate_addr (void *arg)
{
  struct thread *current_thread = thread_current ();
  uint32_t *ptr = (uint32_t *) arg;
  return arg != NULL && is_user_vaddr (ptr) && pagedir_get_page (current_thread->pagedir, ptr) != NULL;
}

static bool
is_valid_string (char *ustr) {
    char *kstr = pagedir_get_page (thread_current ()->pagedir, ustr);
    return kstr != NULL && validate_addr (ustr + strlen (kstr) + 1);
}

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&fs_lock);
}


static int create_fd(struct file *file_) {
  #ifdef USERPROG
    struct thread *t = thread_current ();
    struct file_descriptor *fd = malloc (sizeof (struct file_descriptor));
    fd->file = file_;
    fd->fid = t->next_fid++;
    list_push_back (&t->fd_list, &fd->fd_elem);
    return fd->fid;
  #endif
}

struct file_descriptor *get_file_descriptor (fid_t fid)
{
  #ifdef USERPROG
    struct list *list_ = &thread_current ()->fd_list;

    if (list_empty(list_))
        return NULL;

    struct list_elem *elem = list_begin (list_);
    for (; elem != list_end (list_); elem = list_next (elem))
      {
        struct file_descriptor *fd = list_entry (elem, struct file_descriptor, fd_elem);
        if (fd->fid == fid)
          return fd;
      }
    return NULL;
  #endif
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  uint32_t* args = ((uint32_t*) f->esp);

  if (!validate_addr (args) || !validate_addr (args + 1) || !validate_addr (args + 2) || !validate_addr (args + 3))
    {
      f->eax = -1;
      exit (-1);
    }

  /*
   * The following print statement, if uncommented, will print out the syscall
   * number whenever a process enters a system call. You might find it useful
   * when debugging. It will cause tests to fail, however, so you should not
   * include it in your final submission.
   */

  /* printf("System call number: %d\n", args[0]); */

  if (args[0] == SYS_EXIT)
    {
      f->eax = args[1];
      exit (args[1]);
    }
  else if (args[0] == SYS_PRACTICE)
    {
      f->eax = args[1] + 1;
    }
  else if (args[0] == SYS_CREATE)
    {
      if (args[1] == NULL || !validate_addr (args[1]))
        {
          f->eax = -1;
          exit (-1);
        }
      else
        f->eax = filesys_create ((char *) args[1], args[2]);
    }
  else if (args[0] == SYS_REMOVE)
    {
      if (args[1] == NULL || !validate_addr (args[1]))
        {
          f->eax = -1;
          exit (-1);
        }
      else
        f->eax = filesys_remove ((char *) args[1]);
    }
  else if (args[0] == SYS_OPEN)
    {
      if (args[1] == NULL || !validate_addr (args[1]))
        {
          f->eax = -1;
          exit (-1);
        }
      else {
        struct file* file_ = filesys_open((char*) args[1]);
        if (file_ == NULL)
          f->eax = -1;
        else
          f->eax = create_fd(file_);
      }
    }
  else if (args[0] == SYS_CLOSE)
    {
      if (args[1] < 2)
        {
          f->eax = -1;
          exit (-1);
        }
      else
        {
          struct file_descriptor *fd = get_file_descriptor (args[1]);
          if (fd == NULL)
              f->eax = -1;
          else
            {
              file_close (fd->file);
              list_remove (&fd->fd_elem);
              free (fd);
              f->eax = 0;
            }
        }
    }
  else if (args[0] == SYS_FILESIZE)
    {
      if (args[1] < 2)
        {
          f->eax = -1;
          exit (-1);
        }
      else
        {
          struct file_descriptor *fd = get_file_descriptor (args[1]);
          if (fd == NULL)
            f->eax = -1;
          else
            f->eax = file_length (fd->file);
        }
    }
  else if (args[0] == SYS_WRITE)
    {
      if (args[2] == NULL || !validate_addr (args[2]))
        {
          f->eax = -1;
          exit (-1);
        }
      else if (args[3] < 1)
          f->eax = 0;
      else
        {
          fid_t fid = args[1];
          const void *buffer = (void *) args[2];
          unsigned size = args[3];

          lock_acquire (&fs_lock);

          if (fid == STDOUT_FILENO)
            {
              putbuf ((const char *) args[2], size);
              f->eax = size;
            }
          else if (fid == STDIN_FILENO)
            {
              f->eax = -1;
              exit (-1);
            }
          else
            {
              struct file_descriptor *fd = get_file_descriptor (fid);
              if (fd == NULL)
                  f->eax = -1;
              else
                  f->eax = file_write (fd->file, buffer, size);
            }
          
          lock_release (&fs_lock);
        }
    }
  else if (args[0] == SYS_READ)
    {
      if (args[2] == NULL || !validate_addr (args[2]))
        {
          f->eax = -1;
          exit (-1);
        }
      else if (args[3] < 1)
          f->eax = 0;
      else
        {
          fid_t fid = args[1];
          uint8_t *buffer = (uint8_t *) args[2];
          unsigned size = args[3];

          lock_acquire (&fs_lock);
          
          if (fid == STDIN_FILENO)
            {
              for (unsigned i = 0; i < size; i++)
                {
                  buffer[i] = input_getc ();
                }
              f->eax = size;
            }
          else if (fid == STDOUT_FILENO)
            {
              f->eax = -1;
              exit (-1);
            }
          else
            {
              struct file_descriptor *fd = get_file_descriptor (fid);
              if (fd == NULL)
                  f->eax = -1;
              else
                  f->eax = file_read (fd->file, buffer, size);
            }
          lock_release (&fs_lock);

        }
    }
  else if (args[0] == SYS_SEEK)
    {
      if (args[1] < 2 || args[2] < 0)
        {
          f->eax = -1;
          exit (-1);
        }
      else
        {
          fid_t fid = args[1];
          unsigned position = args[2];
          struct file_descriptor *fd = get_file_descriptor (fid);
          if (fd != NULL)
              file_seek (fd->file, position);
        }
    }
  else if (args[0] == SYS_TELL)
    {
      if (args[1] < 2)
        {
          f->eax = -1;
          exit (-1);
        }
      else
        {
          fid_t fid = args[1];
          unsigned position = args[2];
          struct file_descriptor *fd = get_file_descriptor (fid);
          if (fd == NULL)
              f->eax = -1;
          else
              f->eax = file_tell (fd->file);
        }
    }
  else if (args[0] == SYS_HALT)
    {
      shutdown_power_off();
    }
  else if (args[0] == SYS_EXEC)
    {
      if (!is_valid_string ((char*) args[1]))
        {
          f->eax = -1;
          exit (-1);
        }
      else
        f->eax = process_execute ((char*) args[1]);
    }
  else if (args[0] == SYS_WAIT)
    {
      f->eax = process_wait((tid_t) args[1]);
    }
  else
    {
      f->eax = -1;
      exit (-1);
    }
}
