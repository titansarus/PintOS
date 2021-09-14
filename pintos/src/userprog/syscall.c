#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "lib/string.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/directory.h"
#include "filesys/inode.h"
#include "devices/input.h"
#include "devices/shutdown.h"
#ifdef USERPROG
#include "userprog/exception.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#endif

static void syscall_handler (struct intr_frame *);
static int create_fd (struct file *);
struct file_descriptor *get_file_descriptor (fid_t);
static void sys_exit (struct intr_frame *, uint32_t);
static void sys_practice (struct intr_frame *, uint32_t);
static void sys_create (struct intr_frame *, const char *, off_t);
static void sys_remove (struct intr_frame *, const char *);
static void sys_open (struct intr_frame *, const char *);
static void sys_close (struct intr_frame *, fid_t);
static void sys_filesize (struct intr_frame *, fid_t);
static void sys_write (struct intr_frame *, fid_t, const char *, unsigned);
static unsigned inputbuf (char *, unsigned);
static void sys_read (struct intr_frame *, fid_t, char *, unsigned);
static void sys_seek (fid_t, unsigned);
static void sys_tell (struct intr_frame *, fid_t);
static void sys_exec (struct intr_frame *, char *);
static void sys_mkdir (struct intr_frame *, char *);
static void sys_chdir (struct intr_frame *, char *);
static void sys_readdir (struct intr_frame *, fid_t, char *);
static void sys_isdir (struct intr_frame *, fid_t);
static void sys_inumber (struct intr_frame *, fid_t);





/* Validate arguments for all syscalls */
static bool
validate_addr (const void *arg)
{
  uint32_t *ptr = (uint32_t *) arg;
  return arg != NULL && is_user_vaddr (ptr)

#ifdef USERPROG
         && pagedir_get_page (thread_current ()->pagedir, ptr) != NULL
#endif
         ;
}

static bool
is_valid_string (char *ustr)
{
#ifdef USERPROG
  char *kstr = pagedir_get_page (thread_current ()->pagedir, ustr);
  return kstr != NULL && validate_addr (ustr + strlen (kstr) + 1);
#else
  return true;
#endif
}

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


/* Validate arguments for all syscalls */
static bool
validate_arg (const void *arg,size_t size)
{
    return validate_addr(arg) && validate_addr(arg+size);
}

static bool
validate_args(uint32_t *args){
  if (!validate_arg (args,sizeof (uint32_t)))return false;

  switch (*args) {
    case SYS_READ:
    case SYS_WRITE:
      if (!validate_arg (args+3,sizeof (uint32_t)))return false;

    case SYS_CREATE:
    case SYS_SEEK:
    case SYS_READDIR:
      if (!validate_arg (args+2,sizeof (uint32_t)))return false;
    
    case SYS_PRACTICE:
    case SYS_EXIT:
    case SYS_EXEC:
    case SYS_WAIT:
    case SYS_REMOVE:
    case SYS_OPEN:
    case SYS_FILESIZE:
    case SYS_TELL:
    case SYS_ISDIR:
    case SYS_INUMBER:
    case SYS_MKDIR:
    case SYS_CHDIR:
    case SYS_CLOSE:
      if (!validate_arg (args+1,sizeof (uint32_t)))return false;
  }
  
  return true;
}

static void
syscall_handler (struct intr_frame *f)
{
  uint32_t *args = ((uint32_t *) f->esp);

  if (!validate_args(args)){
    sys_exit(f,-1);
  }


  /*
   * The following print statement, if uncommented, will print out the syscall
   * number whenever a process enters a system call. You might find it useful
   * when debugging. It will cause tests to fail, however, so you should not
   * include it in your final submission.
   */

  /* printf("System call number: %d\n", args[0]); */
  
  uint32_t syscall_code = args[0];

  switch (syscall_code)
    {
      case SYS_EXIT:sys_exit (f, args[1]);
      break;
      case SYS_PRACTICE:sys_practice (f, args[1]);
      break;
      case SYS_CREATE:sys_create (f, (char *) args[1], args[2]);
      break;
      case SYS_REMOVE:sys_remove (f, (char *) args[1]);
      break;
      case SYS_OPEN:sys_open (f, (char *) args[1]);
      break;
      case SYS_CLOSE:sys_close (f, (fid_t) args[1]);
      break;
      case SYS_FILESIZE:sys_filesize (f, (fid_t) args[1]);
      break;
      case SYS_WRITE:sys_write (f, (fid_t) args[1], (char *) args[2], (unsigned) args[3]);
      break;
      case SYS_READ:sys_read (f, (fid_t) args[1], (char *) args[2], (unsigned) args[3]);
      break;
      case SYS_SEEK:sys_seek ((fid_t) args[1], (unsigned) args[2]);
      break;
      case SYS_TELL:sys_tell (f, (fid_t) args[1]);
      break;
      case SYS_HALT:shutdown_power_off ();
      break;
      case SYS_EXEC:sys_exec (f, (char *) args[1]);
      break;
      case SYS_WAIT:f->eax = process_wait ((tid_t) args[1]);
      break;
      case SYS_MKDIR:sys_mkdir(f, (char *) args[1]);
      break;
      case SYS_CHDIR:sys_chdir(f, (char *) args[1]);
      break;
      case SYS_READDIR:sys_readdir(f, (fid_t) args[1], (char *) args[2]);
      break;
      case SYS_ISDIR:sys_isdir(f, (fid_t) args[1]);
      break;
      case SYS_INUMBER:sys_inumber(f, (fid_t) args[1]);
      break;
      default:sys_exit (f, args[1]);
    }
}

static int create_fd (struct file *file_)
{
#ifdef USERPROG
  struct thread *t = thread_current ();
  struct file_descriptor *fd = malloc (sizeof (struct file_descriptor));
  fd->file = file_;
  fd->dir = NULL;
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
    f->eax = filesys_create (file_name, initial_size, false);
}

static void
sys_remove (struct intr_frame *f, const char *file_name)
{
  if (!validate_addr (file_name))
    sys_exit (f, -1);
  else
    f->eax = filesys_remove (file_name);
}

static void
sys_open (struct intr_frame *f, const char *file_name)
{
  if (!validate_addr (file_name))
    sys_exit (f, -1);
  else
    {
      struct file *file_ = filesys_open (file_name);
      f->eax = -1;
      if (file_)
        {
          fid_t fid = create_fd (file_);
          struct inode *inode = file_get_inode (file_);
          if (inode && inode_is_dir (inode))
            get_file_descriptor (fid)->dir = dir_open (inode_reopen (inode));
          f->eax = fid;
        }
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
      f->eax = -1;
      if (fd)
        {
          file_close (fd->file);
          if (fd->dir) dir_close (fd->dir);
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
      f -> eax = fd ? file_length (fd->file) : -1;
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
          f->eax = -1;
          if (fd)
            {
              struct inode *inode = file_get_inode (fd->file);
              f->eax = inode_is_dir (inode) ? -1 : file_write (fd->file, buffer, size);
            }
        }
    }
}

static unsigned
inputbuf (char *buffer, unsigned size)
{
  size_t i = 0;
  while (i < size)
    {
      buffer[i] = input_getc ();
      if (buffer[i++] == '\n')
        {
          buffer[i - 1] = '\0';
          break;
        }
    }
  return i;
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
        f->eax = inputbuf (buffer, size);
      else
        {
          struct file_descriptor *fd = get_file_descriptor (fid);
          f->eax = fd ? file_read (fd->file, buffer, size):  -1 ;
        }
    }
}

static void
sys_seek (fid_t fid, unsigned position)
{
  if (fid < 2)
    exit (-1);
  else
    {
      struct file_descriptor *fd = get_file_descriptor (fid);
      if (fd)
        file_seek (fd->file, position);
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
      f->eax = fd ? file_tell (fd->file) : -1;
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


static void sys_mkdir (struct intr_frame *f, char *name)
{
    if (!validate_addr (name))
    sys_exit (f, -1);
    else{
      f->eax = filesys_create (name, 0, true);
    }
}

static void sys_chdir (struct intr_frame *f, char *name)
{
  if (!validate_addr (name))
    sys_exit (f, -1);
  else{
    struct dir *dir = dir_open_path (name);
    f->eax = false;
    if (dir)
      {
        struct thread *cur = thread_current ();
        dir_close (cur->working_dir);
        cur->working_dir = dir;
        f->eax = true;
      }
  }
}

static void sys_readdir (struct intr_frame *f, fid_t fid, char *name)
{
  if (!validate_addr (name))
    sys_exit (f, -1);
  else{
    struct file *file = get_file_descriptor (fid)->file;
    f->eax = -1;
    if (file)
      {
        struct inode *inode = file_get_inode (file);
        if (!inode || !inode_is_dir (inode))
          f->eax = false;
        else
          {
            struct dir *dir = get_file_descriptor (fid)->dir;
            f->eax = dir_readdir (dir, name);
          }
      }
  }
}

static void sys_isdir (struct intr_frame *f, fid_t fid)
{
  struct file *file = get_file_descriptor (fid)->file;
  f->eax = file ? inode_is_dir (file_get_inode (file)) : -1;
}

static void sys_inumber (struct intr_frame *f, fid_t fid)
{
  struct file *file = get_file_descriptor (fid)->file;
  f->eax = file ? (block_sector_t) inode_get_inumber (file_get_inode (file)) : -1;
}
