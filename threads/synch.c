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
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

void donate_priority(void);
static struct list_elem *list_find(struct list *list, struct list_elem *elem);

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
   decrement it.

   - up or "V": increment the value (and wake up one waiting
   thread, if any). */
void sema_init(struct semaphore *sema, unsigned value)
{
	ASSERT(sema != NULL);

	sema->value = value;
	list_init(&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. This is
   sema_down function. */
void sema_down(struct semaphore *sema)
{
	enum intr_level old_level;
	ASSERT(sema != NULL);
	ASSERT(!intr_context());

	old_level = intr_disable();
	while (sema->value == 0)
	{
		list_push_back(&sema->waiters, &thread_current()->elem);
		thread_block();
	}
	thread_current()->wait_on_lock = NULL;
	sema->value--;

	intr_set_level(old_level);
}
/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool sema_try_down(struct semaphore *sema)
{
	enum intr_level old_level;
	bool success;

	ASSERT(sema != NULL);

	old_level = intr_disable();
	if (sema->value > 0)
	{
		sema->value--;
		success = true;
	}
	else
		success = false;
	intr_set_level(old_level);

	return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void sema_up(struct semaphore *sema)
{
	enum intr_level old_level;

	ASSERT(sema != NULL);
	struct thread *next_holder = NULL;
	old_level = intr_disable();
	if (!list_empty(&sema->waiters))
	{
		struct list_elem *max_elem = list_max(&sema->waiters, compare, NULL);
		next_holder = list_entry(max_elem, struct thread, elem);

		struct thread *curr = thread_current();

		for (struct list_elem *waiter_elem = list_begin(&sema->waiters); waiter_elem != list_end(&sema->waiters); waiter_elem = list_next(waiter_elem))
		{
			struct thread *waiter_thread = list_entry(waiter_elem, struct thread, elem);

			if (list_find(&curr->donations, &waiter_thread->donation_elem))
			{
				list_remove(&waiter_thread->donation_elem);

				if (list_empty(&curr->donations))
				{
					curr->priority = curr->priority_origin;
					break;
				}

				struct list_elem *donation_max_elem = list_max(&curr->donations, compare, NULL);
				struct thread *donation_max_thread = list_entry(donation_max_elem, struct thread, donation_elem);
				curr->priority = donation_max_thread->priority;
			}
		}

		list_remove(max_elem);

		thread_unblock(next_holder);
	}

	sema->value++;

	/*intr_context()를 넣어준 이유는 넣지않은상태에선 kernel panic이 뜨면서
	thread_yield()함수에서 !intr_context() failed가 뜨기때문.
	인터럽트 컨텍스트에서 yield를 호출하면 안되는이유
	1. 재진입 문제 : 인터럽트 핸들러는 재진입 불가능하다고 가정하는데, 이는 동일한 인터럽트 핸들러가
		동시에 여러번 호출되어 실행되지 않는다는것임.
		그러나 yield를 호출하면 다른 스레드나 프로세스가 실행될수 있어 동일한 인터럽트 핸들러가 재진입할 수 있는
		상황이 발생할 수 있음.
	2. 스택 문제 : 인터럽트 핸들러는 종종 별도의 스택을 사용하여 실행되는데, yield를 호출하면
		현재 스택 컨텍스트를 변경할 수 있으므로 인터럽트 핸들러가 반환될 때 스택상태에
		문제가 발생할 수 있음.
	3. 우선순위 문제 : 인터럽트는 종종 시스템 내 중요한 사건을 나타내므로 즉시 처리되어야 함.
		인터럽트 처리중에 yield를 호출하면 인터럽트 처리가 지연될 수 있어 이로인해
		시스템의 반응성이 저하될 수 있음.
	*/
	if (next_holder && thread_current()->priority <= next_holder->priority && !(intr_context()))
	{
		thread_yield();
	}

	// struct thread *next = NULL;
	// if (!list_empty (&sema->waiters))   {// waiters에 들어있을 때
	//     list_sort(&sema->waiters, compare_priority, NULL);
	//     next = list_entry (list_pop_front (&sema->waiters), struct thread, elem);
	//     thread_unblock (next);  // unblock처리 -> ready list로 옮겨줌
	// }
	// sema->value++;  // sema 값 증가
	// if (next && next->priority > thread_current()->priority && !(intr_context())) {
	//     thread_yield();
	// }
	intr_set_level(old_level);
}

static void sema_test_helper(void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void sema_self_test(void)
{
	struct semaphore sema[2];
	int i;

	printf("Testing semaphores...");
	sema_init(&sema[0], 0);
	sema_init(&sema[1], 0);
	thread_create("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
	for (i = 0; i < 10; i++)
	{
		sema_up(&sema[0]);
		sema_down(&sema[1]);
	}
	printf("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper(void *sema_)
{
	struct semaphore *sema = sema_;
	int i;

	for (i = 0; i < 10; i++)
	{
		sema_down(&sema[0]);
		sema_up(&sema[1]);
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
void lock_init(struct lock *lock)
{
	ASSERT(lock != NULL);

	lock->holder = NULL;
	sema_init(&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void lock_acquire(struct lock *lock)
{
	ASSERT(lock != NULL);
	ASSERT(!intr_context());
	ASSERT(!lock_held_by_current_thread(lock));
	struct thread *curr = thread_current();
	curr->wait_on_lock = lock;

	if (lock->holder != NULL)
	{

		if (curr->priority > lock->holder->priority)
		{
			list_push_back(&lock->holder->donations, &thread_current()->donation_elem);

			donate_priority();
		}
	}

	sema_down(&lock->semaphore);

	curr->wait_on_lock = NULL;

	lock->holder = curr;
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool lock_try_acquire(struct lock *lock)
{
	bool success;

	ASSERT(lock != NULL);
	ASSERT(!lock_held_by_current_thread(lock));

	success = sema_try_down(&lock->semaphore);
	if (success)
		lock->holder = thread_current();
	return success;
}

/* Releases LOCK, which must be owned by the current thread.
   This is lock_release function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void lock_release(struct lock *lock)
{
	ASSERT(lock != NULL);
	ASSERT(lock_held_by_current_thread(lock));

	lock->holder = NULL;
	sema_up(&lock->semaphore);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool lock_held_by_current_thread(const struct lock *lock)
{
	ASSERT(lock != NULL);

	return lock->holder == thread_current();
}

/* One semaphore in a list. */
struct semaphore_elem
{
	struct list_elem elem;		/* List element. */
	struct semaphore semaphore; /* This semaphore. */
};

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void cond_init(struct condition *cond)
{
	ASSERT(cond != NULL);

	list_init(&cond->waiters);
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
void cond_wait(struct condition *cond, struct lock *lock)
{
	struct semaphore_elem waiter;

	ASSERT(cond != NULL);
	ASSERT(lock != NULL);
	ASSERT(!intr_context());
	ASSERT(lock_held_by_current_thread(lock));

	sema_init(&waiter.semaphore, 0);
	list_push_back(&cond->waiters, &waiter.elem);
	lock_release(lock);
	sema_down(&waiter.semaphore);
	lock_acquire(lock);
}

bool compare_cond(const struct list_elem *a, const struct list_elem *b, void *aux)
{
	struct semaphore_elem *t_a = list_entry(a, struct semaphore_elem, elem);
	struct semaphore_elem *t_b = list_entry(b, struct semaphore_elem, elem);

	struct list_elem *elem_a = list_max(&t_a->semaphore.waiters, compare, NULL);
	struct list_elem *elem_b = list_max(&t_b->semaphore.waiters, compare, NULL);

	struct thread *ta = list_entry(elem_a, struct thread, elem);
	struct thread *tb = list_entry(elem_b, struct thread, elem);
	return ta->priority < tb->priority;
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void cond_signal(struct condition *cond, struct lock *lock UNUSED)
{
	ASSERT(cond != NULL);
	ASSERT(lock != NULL);
	ASSERT(!intr_context());
	ASSERT(lock_held_by_current_thread(lock));

	if (!list_empty(&cond->waiters))
	{	
		struct list_elem *max_elem = list_max(&cond->waiters, compare_cond, NULL);
		sema_up(&list_entry(max_elem,
							struct semaphore_elem, elem)->semaphore);
		list_remove(max_elem);
	}
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void cond_broadcast(struct condition *cond, struct lock *lock)
{
	ASSERT(cond != NULL);
	ASSERT(lock != NULL);

	while (!list_empty(&cond->waiters))
		cond_signal(cond, lock);
}

void donate_priority(void)
{
	struct thread *holder = thread_current()->wait_on_lock->holder;
	while (holder != NULL)
	{
		holder->priority = thread_current()->priority;
		if (holder->wait_on_lock == NULL)
			break;
		holder = holder->wait_on_lock->holder;
	}
}

static struct list_elem *list_find(struct list *list, struct list_elem *elem)
{
	struct list_elem *curr;
	for (curr = list_begin(list); curr != list_end(list); curr = list_next(curr))
	{
		if (curr == elem)
			return curr;
	}
	return NULL;
}