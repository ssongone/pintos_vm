#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include "threads/thread.h"

void syscall_init (void);

void check_addr(const uint64_t *addr);

void exit(int status);
void call_exit(struct thread* curr, uint64_t status);
int call_write(const void *buffer, unsigned size);
bool call_create (const char *file, unsigned initial_size);
void call_halt(void);
int call_open(const char *file);
int add_file_to_fdt(struct file *file);


#endif /* userprog/syscall.h */
