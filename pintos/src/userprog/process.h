#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

typedef int pid_t;
typedef int fid_t;

struct file_descriptor
  {
    fid_t fid;
    struct file *file;
    struct dir *dir;
    struct list_elem fd_elem;
  };

struct process_status
  {
    /* tid of the curresponing process */
    pid_t pid;

    int exit_code;
    bool is_exited;

    /* wait sema
    * init to 0
    * used for handling wait between parent and child thread
    */
    struct semaphore ws;

    /* initially 2
    * used for freeing resources 
    * number of alive threads referencing to this status  
    */
    int rc;

    /* strictly for changing value of rc */
    struct lock rc_lock;

    struct list_elem children_elem;
  };

void init_process_status (struct process_status *);

void decrease_rc (struct process_status *);
struct process_status *find_child (struct thread *, tid_t);
#endif /* userprog/process.h */
