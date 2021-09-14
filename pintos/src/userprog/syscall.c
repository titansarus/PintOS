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

static void syscall_handler (struct intr_frame *);

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
sys_exit (struct intr_frame *f, uint32_t code)
{
  f->eax = code;
  exit (code);
}

static void
sys_practice (struct intr_frame *f, uint32_t input)
{
  f->eax = input + 1;
}

static void
sys_create (struct intr_frame *f, const char *file_name, off_t initial_size)
{
  if (!validate_addr (file_name))
      sys_exit (f, -1);
  else
      f->eax = filesys_create_l (file_name, initial_size);
}

static void
sys_remove (struct intr_frame *f, const char *file_name)
{
  if (!validate_addr (file_name))
      sys_exit (f, -1);
  else
      f->eax = filesys_remove_l (file_name);
}

static void
sys_open (struct intr_frame *f, const char *file_name)
{
  if (!validate_addr (file_name))
      sys_exit (f, -1);
  else 
    {
      struct file* file_ = filesys_open_l (file_name);
      if (file_ == NULL)
          f->eax = -1;
      else
          f->eax = create_fd (file_);
    }
}

static void
sys_close (struct intr_frame *f, fid_t fid)
{
  if (fid < 2) /* Trying to close stdin or stdout */
      sys_exit (f, -1);
  else
    {
      struct file_descriptor *fd = get_file_descriptor (fid);
      if (fd == NULL)
          f->eax = -1;
      else
        {
          file_close_l(fd->file);
          list_remove (&fd->fd_elem);
          free (fd);
          f->eax = 0;
        }
    }
}

static void
sys_filesize (struct intr_frame *f, fid_t fid)
{
  if (fid < 2) /* stdin or stdout */
      sys_exit (f, -1);
  else
    {
      struct file_descriptor *fd = get_file_descriptor (fid);
      if (fd == NULL)
          f->eax = -1;
      else
          f->eax = file_length_l (fd->file);
    }
}

static void
sys_write (struct intr_frame *f, fid_t fid, const char *buffer, unsigned size)
{
  if (fid == STDIN_FILENO || !validate_addr (buffer))
      sys_exit (f, -1);
  else if (size < 1)
      f->eax = 0;
  else
    {
      if (fid == STDOUT_FILENO)
        {
          putbuf (buffer, size);
          f->eax = size;
        }
      else
        {
          struct file_descriptor *fd = get_file_descriptor (fid);
          if (fd == NULL)
              f->eax = -1;
          else
              f->eax = file_write_l (fd->file, buffer, size);
        }
    }
}

static unsigned
inputbuf (char * buffer, unsigned size)
{
  for (unsigned i = 0; i < size; i++)
    {
      buffer[i] = input_getc ();
    }
  return size;
}

static void
sys_read (struct intr_frame *f, fid_t fid, char *buffer, unsigned size)
{
  if (fid == STDOUT_FILENO || !validate_addr (buffer))
      sys_exit (f, -1);
  else if (size < 1)
      f->eax = 0;
  else
    {
      if (fid == STDIN_FILENO)
          f-> eax = inputbuf (buffer, size);
      else
        {
          struct file_descriptor *fd = get_file_descriptor (fid);
          if (fd == NULL)
              f->eax = -1;
          else
              f->eax = file_read_l (fd->file, buffer, size);
        }
    }
}

static void
sys_seek (struct intr_frame *f, fid_t fid, unsigned position)
{
  if (fid < 2 || position < 0)
      exit (-1);
  else
    {
      struct file_descriptor *fd = get_file_descriptor (fid);
      if (fd != NULL)
          file_seek_l (fd->file, position);
    }
}

static void
sys_tell (struct intr_frame *f, fid_t fid)
{
  if (fid < 2)
      sys_exit (f, -1);
  else
    {
      struct file_descriptor *fd = get_file_descriptor (fid);
      if (fd == NULL)
          f->eax = -1;
      else
          f->eax = file_tell_l (fd->file);
    }
}

static void
sys_exec (struct intr_frame *f, char *command)
{
  if (!is_valid_string (command))
      sys_exit (f, -1);
  else
    f->eax = process_execute (command);
}

static void
syscall_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);

  if (!validate_addr (args) || !validate_addr (args + 1) || !validate_addr (args + 2) || !validate_addr (args + 3))
      sys_exit (f, -1);

  uint32_t syscall_code = args[0];

  /*
   * The following print statement, if uncommented, will print out the syscall
   * number whenever a process enters a system call. You might find it useful
   * when debugging. It will cause tests to fail, however, so you should not
   * include it in your final submission.
   */

  /* printf("System call number: %d\n", args[0]); */

  switch (syscall_code)
  {
  case SYS_EXIT:
    sys_exit (f, args[1]);
    break;
  case SYS_PRACTICE:
    sys_practice (f, args[1]);
    break;
  case SYS_CREATE:
    sys_create (f, (char *) args[1], args[2]);
    break;
  case SYS_REMOVE:
    sys_remove (f, (char *) args[1]);
    break;
  case SYS_OPEN:
    sys_open (f, (char*) args[1]);
    break;
  case SYS_CLOSE:
    sys_close (f, (fid_t) args[1]);
    break;
  case SYS_FILESIZE:
    sys_filesize (f, (fid_t) args[1]);
    break;
  case SYS_WRITE:
    sys_write (f, (fid_t) args[1], (char *) args[2], (unsigned) args[3]);
    break;
  case SYS_READ:
    sys_read (f, (fid_t) args[1], (char *) args[2], (unsigned) args[3]);
    break;
  case SYS_SEEK:
    sys_seek (f, (fid_t) args[1], (unsigned) args[2]);
    break;
  case SYS_TELL:
    sys_tell (f, (fid_t) args[1]);
    break;
  case SYS_HALT:
    shutdown_power_off ();
    break;
  case SYS_EXEC:
    sys_exec (f, (char*) args[1]);
    break;
  case SYS_WAIT:
    f->eax = process_wait((tid_t) args[1]);
    break;
  default:
    sys_exit (f, args[1]);
  }
}
