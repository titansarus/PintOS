/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <stdio.h>
#include <string.h>

static bool lock_priority_comparator (const struct list_elem *, const struct list_elem *, void *);
static bool sema_priority_comparator (const struct list_elem *, const struct list_elem *, void *);

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void sema_init (struct semaphore *sema, unsigned value)
{
  ASSERT (sema != NULL);

  sema->value = value;
  list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void sema_down (struct semaphore *sema) /*(0 w 0)*/
{
  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  enum intr_level old_level = intr_disable ();

  while (!sema->value)
    {
      list_insert_ordered (&sema->waiters, &thread_current ()->elem, thread_priority_comparator, NULL);
      thread_block ();
    }
  sema->value--;

  intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool sema_try_down (struct semaphore *sema)
{
  enum intr_level old_level;
  bool success;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (sema->value > 0)
    {
      sema->value--;
      success = true;
    }
  else
    success = false;
  intr_set_level (old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void sema_up (struct semaphore *sema) /*(0 w 0)*/
{
  ASSERT (sema);

  enum intr_level old_level = intr_disable ();

  struct thread *waited_thread = NULL;
  struct thread *cur = thread_current ();

  /* Unblocking all threads which have waited on the sema */
  while (!list_empty (&sema->waiters))
    {
      waited_thread = list_entry (list_pop_front (&sema->waiters), struct thread, elem);
      thread_unblock (waited_thread);
    }

  sema->value++;

  /* If we have unblocked a thread that has higher prioirty than the currently runnig thread, yield. */
  if (waited_thread && cur->priority < waited_thread->priority)
    thread_yield_ultra (cur);

  intr_set_level (old_level);
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void sema_self_test (void)
{
  struct semaphore sema[2];
  int i;

  printf ("Testing semaphores...");
  sema_init (&sema[0], 0);
  sema_init (&sema[1], 0);
  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++)
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
  printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_)
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++)
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
    }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void lock_init (struct lock *lock) /*(0 w 0)*/
{
  ASSERT (lock);

  lock->holder = NULL;
  sema_init (&lock->semaphore, 1);

  /* Locks have a priority of -1 at initialization time. */
  lock->priority = BASE_PRIORITY;
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void lock_acquire (struct lock *lock) /*(0 w 0)*/
{
  ASSERT (lock);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));

  enum intr_level old_level = intr_disable ();

  struct thread *cur = thread_current ();
  int iterated = 0;
  cur->required_lock = lock;

  /* do priority donation for an specified depth, i.e. LOCK_LEVEL to prevent infinite donations. */
  while (iterated++ < LOCK_LEVEL)
    {

      /* If the lock we want to acquire doesn't have any holder, we don't need any donation. */
      if (!cur->required_lock->holder)
        break;

      /* If the lock we want to acquire belongs to a thread that has higher priority than us, we don't need any donation. */
      if (cur->required_lock->priority >= cur->priority)
        break;

      /* priority donation: change priority of holder of required lock and set lock's priority*/
      thread_set_priority_on_given (cur->required_lock->holder, cur->priority, true);
      cur->required_lock->priority = cur->priority;

      /* If the donee thread is not waiting on another lock, then we don't need nested donation. */
      if (!cur->required_lock->holder->required_lock)
        break;

      /* handle nested donation for next iteration */
      cur = cur->required_lock->holder;
    }

  sema_down (&lock->semaphore);

  cur = thread_current ();
  cur->required_lock = NULL;

  /* Add this lock to the thread's lock holding list */
  list_insert_ordered (&cur->acquired_locks, &lock->elem_lock,
                       lock_priority_comparator, NULL);

  lock->holder = thread_current ();

  intr_set_level (old_level);
}

static bool
lock_priority_comparator (const struct list_elem *a, const struct list_elem *b, void *aux UNUSED) /*(0 w 0)*/
{
  const struct lock *thread_a = list_entry (a, struct lock, elem_lock);
  const struct lock *thread_b = list_entry (b, struct lock, elem_lock);

  return thread_a->priority >= thread_b->priority;
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool lock_try_acquire (struct lock *lock)
{
  bool success;

  ASSERT (lock != NULL);
  ASSERT (!lock_held_by_current_thread (lock));

  success = sema_try_down (&lock->semaphore);
  if (success)
    lock->holder = thread_current ();
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void lock_release (struct lock *lock) /*(0 w 0)*/
{
  ASSERT (lock);
  ASSERT (lock_held_by_current_thread (lock));

  struct thread *cur = thread_current ();

  enum intr_level old_level = intr_disable ();

  lock->holder = NULL;
  sema_up (&lock->semaphore);

  list_remove (&lock->elem_lock);
  lock->priority = BASE_PRIORITY;

  /* if current thread didn't acquire any other lock, then we revert its priority to base.  */
  if (list_empty (&cur->acquired_locks))
    {
      cur->is_donated = false;
      thread_set_priority (cur->base_priority);
      goto finish;
    }

  /* If there are more locks that our thread acquired, get the first one with the highest priority.
    If no other thread waits on that lock, its priority is BASE_PRIORITY and we don't need to to do anything special
    But if there exists some thread that has waited on the lock,
    we must revert the current thread priority to previous donor thread priority. */
  struct lock *lock_first = list_entry (list_front (&cur->acquired_locks), struct lock, elem_lock);
  if (lock_first->priority != BASE_PRIORITY)
    {
      thread_set_priority_on_given (cur, lock_first->priority, true);
      goto finish;
    }
  thread_set_priority (cur->base_priority);

finish:
  intr_set_level (old_level);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool lock_held_by_current_thread (const struct lock *lock)
{
  ASSERT (lock != NULL);

  return lock->holder == thread_current ();
}

/* One semaphore in a list. */
struct semaphore_elem /*(0 w 0)*/
  {
    struct list_elem elem;              /* List element. */
    struct semaphore semaphore;         /* This semaphore. */
    int priority;                       /* Semaphore priority for conditional variables. */
  };

static bool
sema_priority_comparator (const struct list_elem *a, const struct list_elem *b, void *aux UNUSED) /*(0 w 0)*/
{
  const struct semaphore_elem *sema_a = list_entry (a, struct semaphore_elem, elem);
  const struct semaphore_elem *sema_b = list_entry (b, struct semaphore_elem, elem);

  return (sema_a->priority > sema_b->priority);
}

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void cond_init (struct condition *cond) /*(0 w 0)*/
{
  ASSERT (cond);

  list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void cond_wait (struct condition *cond, struct lock *lock) /*(0 w 0)*/
{
  ASSERT (cond);
  ASSERT (lock);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  struct semaphore_elem waiter;
  sema_init (&waiter.semaphore, 0);
  waiter.priority = thread_current ()->priority;
  list_insert_ordered (&cond->waiters, &waiter.elem, sema_priority_comparator, NULL);
  lock_release (lock);
  sema_down (&waiter.semaphore);
  lock_acquire (lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void cond_signal (struct condition *cond, struct lock *lock UNUSED)
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters))
    sema_up (&list_entry (list_pop_front (&cond->waiters),
                          struct semaphore_elem, elem)
                ->semaphore);
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void cond_broadcast (struct condition *cond, struct lock *lock)
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);

  while (!list_empty (&cond->waiters))
    cond_signal (cond, lock);
}
