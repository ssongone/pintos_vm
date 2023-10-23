#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include "threads/thread.h"

struct res_data
{
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rdx;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;

	struct intr_frame *f;
};
void syscall_init (void);

void check_addr(const uint64_t *addr);
struct file *find_file_by_Fd(int fd);
int add_file_to_fdt(struct file *file);


void call_close(struct res_data res_data);
void call_exit(struct res_data res_data);
int call_write(struct res_data res_data);
bool call_create (struct res_data res_data);
void call_halt(struct res_data res_data);
int call_open(struct res_data res_data);
int call_read(struct res_data res_data);
int call_filesize(struct res_data res_data);
int call_wait(struct res_data res_data);
int call_fork (struct res_data res_data);
int call_exec (struct res_data res_data);
bool call_remove(struct res_data res_data);
void call_seek(struct res_data res_data);
unsigned call_tell(struct res_data res_data);
void * call_mmap(struct res_data res_data);
void call_munmap (struct res_data res_data);

void exit(uint64_t status);



#endif /* userprog/syscall.h */
