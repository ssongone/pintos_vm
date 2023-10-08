#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include "threads/thread.h"

void syscall_init (void);

void check_addr(const uint64_t *addr);
struct file *find_file_by_Fd(int fd);
int add_file_to_fdt(struct file *file);

void exit(int status);
void call_close(int fd);
void call_exit(struct thread* curr, uint64_t status);
// int call_write(const void *buffer, unsigned size);
int call_write(int fd, const void *buffer, unsigned size);
bool call_create (const char *file, unsigned initial_size);
void call_halt(void);
int call_open(const char *file);
int call_read(int fd, void *buffer, unsigned size);
int call_filesize(int fd);
int call_wait(int pid);
int call_fork (const char *thread_name);
int call_exec (const char *file);
bool call_remove(const char *file);
void call_seek(int fd, unsigned new_pos);
unsigned call_tell(int fd);



#endif /* userprog/syscall.h */
