#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

#include "userprog/process.h"
#include "threads/palloc.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/synch.h"

void syscall_entry(void);
void syscall_handler(struct intr_frame *);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081			/* Segment selector msr */
#define MSR_LSTAR 0xc0000082		/* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

struct semaphore syn_sema;

void syscall_init(void)
{
	sema_init(&syn_sema, 1);
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48 |
							((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t)syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			  FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
void syscall_handler(struct intr_frame *f)
{
	struct thread *curr = thread_current();
	// TODO: Your implementation goes here.

	switch (f->R.rax)
	{
	case SYS_WRITE:
		f->R.rax = call_write(f->R.rdi, f->R.rsi, f->R.rdx);
		break;
	case SYS_EXIT:
		call_exit(curr, f->R.rdi);
		break;
	case SYS_CREATE:
		f->R.rax = call_create(f->R.rdi, f->R.rsi);
		break;
	case SYS_HALT:
		call_halt();
		break;
	case SYS_OPEN:
		f->R.rax = call_open(f->R.rdi);
		break;
	case SYS_CLOSE:
		call_close(f->R.rdi);
		break;
	case SYS_READ:
		f->R.rax = call_read(f->R.rdi, f->R.rsi, f->R.rdx);
		break;
	case SYS_FILESIZE:
		f->R.rax = call_filesize(f->R.rdi);
		break;
	case SYS_WAIT:
		f->R.rax = call_wait(f->R.rdi);
		break;
	case SYS_FORK:
		memcpy(&curr->ptf, f, sizeof(struct intr_frame));
		f->R.rax = call_fork(f->R.rdi);
		break;
	case SYS_EXEC:
		f->R.rax = call_exec(f->R.rdi);
		break;
	case SYS_REMOVE:
		f->R.rax = call_remove(f->R.rdi);
		break;
	case SYS_SEEK:
		call_seek(f->R.rdi, f->R.rsi);
		break;
	case SYS_TELL:
		f->R.rax = call_tell(f->R.rdi);
		break;

	default:
		call_exit(curr, -1);
		break;
	}
}

void call_exit(struct thread *curr, uint64_t status)
{
	curr->exit_status = status;

	printf("%s: exit(%d)\n", curr->name, status);

	thread_exit();
	return;
}

int call_write(int fd, const void *buffer, unsigned size)
{
	check_addr(buffer);
	sema_down(&syn_sema);

	if (fd == 1) // fd 0 : 표준입력, fd 1 : 표준 출력
	{
		putbuf(buffer, size);
		sema_up(&syn_sema);
		return size;
	}
	if (fd == 0)
	{
		sema_up(&syn_sema);
		return -1;
	}
	struct file *file = find_file_by_Fd(fd);
	if (file == NULL)
	{
		call_exit(thread_current(), -1);
	}

	if (file->deny_write)
	{
		sema_up(&syn_sema);
		return 0;
	}
	int write_byte = file_write(file, buffer, size);
	sema_up(&syn_sema);
	return write_byte;
}
bool call_create(const char *file, unsigned initial_size)
{
	check_addr(file);
	return filesys_create(file, initial_size);
}

void call_halt(void)
{
	power_off();
}

int call_open(const char *file)
{
	check_addr(file);

	struct file *open_file = filesys_open(file);

	if (open_file == NULL)
	{
		return -1;
	}

	int fd = add_file_to_fdt(open_file);

	if (fd == -1)
	{
		file_close(open_file);
	}

	return fd;
}

void call_close(int fd)
{

	struct thread *cur = thread_current();
	struct file *file = find_file_by_Fd(fd);
	if (file == NULL)
	{
		return;
	}
	cur->fd_table[fd] = NULL;

	file_close(file);
}

int call_read(int fd, void *buffer, unsigned size)
{
	check_addr(buffer);

	if (fd == 1)
	{
		return -1;
	}
	if (fd == 0)
	{
		int byte = input_getc();
		return byte;
	}

	sema_down(&syn_sema);

	struct file *file = find_file_by_Fd(fd);
	int read_result;

	if (file == NULL)
	{
		sema_up(&syn_sema);
		call_exit(thread_current(), -1);
		return -1;
	}
	else
	{
		read_result = file_read(find_file_by_Fd(fd), buffer, size);
	}
	sema_up(&syn_sema);

	return read_result;
}

int call_wait(tid_t child_tid)
{
	return process_wait(child_tid);
}

int call_filesize(int fd)
{
	struct file *file = find_file_by_Fd(fd);

	if (file == NULL)
	{
		return 0;
	}

	return file_length(file);
}

int call_fork(const char *thread_name)
{
	check_addr(thread_name);

	return process_fork(thread_name, &thread_current()->ptf);
	;
}

int call_exec(const char *file)
{
	char *file_copy;
	check_addr(file);

	file_copy = palloc_get_page(PAL_ZERO);

	if (!file_copy)
	{
		call_exit(thread_current(), -1);
		return -1;
	}

	strlcpy(file_copy, file, strlen(file) + 1);
	if (process_exec(file_copy) == -1)
	{

		call_exit(thread_current(), -1);
		return -1;
	}
}

bool call_remove(const char *file)
{

	check_addr(file);
	sema_down(&syn_sema);
	bool return_ans = filesys_remove(file);
	sema_up(&syn_sema);
	return return_ans;
}

void call_seek(int fd, unsigned new_pos)
{
	struct file *cur = thread_current()->fd_table[fd];

	if (cur != NULL)
	{
		file_seek(cur, new_pos);
	}
}

unsigned call_tell(int fd)
{
	struct file *cur = thread_current()->fd_table[fd];

	if (cur != NULL)
	{
		return file_tell(cur);
	}
}

int add_file_to_fdt(struct file *file)
{
	struct thread *cur = thread_current();
	struct file **fdt = cur->fd_table;

	while (cur->fd_idx < 128 && fdt[cur->fd_idx])
	{
		cur->fd_idx++;
	}

	if (cur->fd_idx >= 128)
	{
		return -1;
	}

	fdt[cur->fd_idx] = file;
	return cur->fd_idx;
}

struct file *find_file_by_Fd(int fd)
{
	struct thread *cur = thread_current();

	if (fd < 0 || fd >= 128)
	{
		return NULL;
	}
	return cur->fd_table[fd];
}

void check_addr(const uint64_t *addr)
{
	struct thread *curr = thread_current();
	if (addr == NULL || !(is_user_vaddr(addr)) || pml4_get_page(curr->pml4, addr) == NULL)
	{
		// exit(-1);
		call_exit(curr, -1);
	}
}
