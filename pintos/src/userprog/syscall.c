#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#ifdef USERPROG
#include "userprog/process.h"
#endif


static void syscall_handler (struct intr_frame *);

/* Validate arguments for all syscalls */
static bool
validate_arg (void *arg)
{
  struct thread *current_thread = thread_current ();
  uint32_t *ptr = (uint32_t *) arg;
  return arg != NULL && is_user_vaddr (ptr) && pagedir_get_page (current_thread->pagedir, ptr) != NULL;
}

bool is_valid_string (char *ustr) {
    char *kstr = pagedir_get_page (thread_current ()->pagedir, ustr);
    return kstr != NULL && validate_arg (ustr + strlen (kstr) + 1);
}

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  uint32_t* args = ((uint32_t*) f->esp);

  if (!validate_arg (args) || !validate_arg (args + 1) || !validate_arg (args + 2) || !validate_arg (args + 3))
    {
      f->eax = -1;
      printf ("%s: exit(%d)\n", &thread_current ()->name, -1);
      thread_exit ();
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
      printf ("%s: exit(%d)\n", &thread_current ()->name, args[1]);
      #ifdef USERPROG
      struct thread* t= thread_current ();
      t->ps->exit_code = args[1];
      t->ps->is_exited = 1;
      #endif

      thread_exit ();
    }
  else if (args[0] == SYS_PRACTICE)
    {
      f->eax = args[1] + 1;
    }
  else if (args[0] == SYS_WRITE)
    {
      /* TODO: implement for all file descriptors */
      int fd = args[1];
      const void *buffer = (void *) args[2];
      unsigned size = args[3];
      if (fd == 1)
        {
          putbuf ((const char *) args[2], size);          
          f->eax = size;
        }
    }
  else if (args[0] == SYS_READ)
    {
      /* TODO: implement for all file descrimtors */
      int fd = args[1];
      uint8_t *buffer = (uint8_t *) args[2];
      unsigned size = args[3];
      if (fd == 0)
        {
          for (unsigned i = 0; i < size; i++)
            {
              buffer[i] = input_getc ();
            }
          f->eax = size;
        }
    }
  else if (args[0] == SYS_HALT)
    {
      shutdown_power_off();
    }
  else if (args[0] == SYS_EXEC)
    {
      if (!is_valid_string (args[1]))
        f->eax = -1;
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
      printf ("%s: exit(%d)\n", &thread_current ()->name, -1);
      thread_exit ();
    }
}
