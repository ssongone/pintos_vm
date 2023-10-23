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



typedef uint64_t (*syscall_handler_t)(struct res_data);

syscall_handler_t syscall_handlers[] = {
    [SYS_WRITE] = call_write,
    [SYS_EXIT] = call_exit,
    [SYS_CREATE] = call_create,
    [SYS_HALT] = call_halt,
    [SYS_OPEN] = call_open,
    [SYS_CLOSE] = call_close,
    [SYS_READ] = call_read,
    [SYS_FILESIZE] = call_filesize,
    [SYS_WAIT] = call_wait,
    [SYS_FORK] = call_fork,
    [SYS_EXEC] = call_exec,
    [SYS_REMOVE] = call_remove,
    [SYS_SEEK] = call_seek,
    [SYS_TELL] = call_tell,
    [SYS_MMAP] = call_mmap,
    [SYS_MUNMAP] = call_munmap,
};

void syscall_handler(struct intr_frame *f) {
	struct thread *curr = thread_current();
	syscall_handler_t handler = syscall_handlers[f->R.rax];

	struct res_data res_data = {
		.rdi = f->R.rdi,
        .rsi = f->R.rsi,
        .rdx = f->R.rdx,
		.r10 = f->R.r10,
		.r9 = f->R.r9,
		.r8 = f->R.r8,
		.f = f
	};

	
	if (handler) {
		f->R.rax = handler(res_data);
		// handler(res_data);
		return;
	}

	exit(-1);
}

void call_exit(struct res_data res_data)
{
	struct thread *curr = thread_current(); 
	uint64_t status = res_data.rdi; 
	curr->exit_status = status;

	printf("%s: exit(%d)\n", curr->name, status);

	thread_exit();
	return;
}

void exit(uint64_t status)
{
	struct thread *curr = thread_current(); 
	curr->exit_status = status;

	printf("%s: exit(%d)\n", curr->name, status);

	thread_exit();
	return;
}

int call_write(struct res_data res_data)
{

	int fd = res_data.rdi;
	const void *buffer = res_data.rsi;
	unsigned size = res_data.rdx;

	sema_down(&syn_sema);
	check_addr(buffer);

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
		sema_up(&syn_sema);
		exit(-1);
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

bool call_create(struct res_data res_data)
{
	const char *file = res_data.rdi;
	unsigned initial_size = res_data.rsi;

	sema_down(&syn_sema);
	check_addr(file);
	bool result = filesys_create(file, initial_size);
	sema_up(&syn_sema);

	return result;
}

void call_halt(struct res_data res_data)
{
	power_off();
}

int call_open(struct res_data res_data)
{
	const char * file = res_data.rdi;
	sema_down(&syn_sema);
	check_addr(file);

	struct file *open_file = filesys_open(file);

	if (open_file == NULL)
	{
		sema_up(&syn_sema);
		return -1;
	}

	int fd = add_file_to_fdt(open_file);

	if (fd == -1)
	{
		file_close(open_file);
	}

	sema_up(&syn_sema);
	return fd;
}

void call_close(struct res_data res_data)
{
	int fd = res_data.rdi;

	sema_down(&syn_sema);

	struct thread *cur = thread_current();
	struct file *file = find_file_by_Fd(fd);
	if (file == NULL)
	{
		sema_up(&syn_sema);
		return;
	}
	cur->fd_table[fd] = NULL;

	file_close(file);
	sema_up(&syn_sema);

}

int call_read(struct res_data res_data)
{
	int fd = res_data.rdi;
	void *buffer = res_data.rsi;
	unsigned size = res_data.rdx;

	sema_down(&syn_sema);
	check_addr(buffer);

	if (fd == 1)
	{
		sema_up(&syn_sema);
		return -1;
	}
	if (fd == 0)
	{
		int byte = input_getc();
		sema_up(&syn_sema);
		return byte;
	}

	struct page *buf_page = spt_find_page(&thread_current()->spt, buffer);
	if (buf_page != NULL && !buf_page->writable)
	{
		sema_up(&syn_sema);
		exit(-1);
	}

	struct file *file = find_file_by_Fd(fd);
	int read_result;

	if (file == NULL)
	{
		sema_up(&syn_sema);
		exit(-1);
		return -1;
	}
	else
	{
		read_result = file_read(find_file_by_Fd(fd), buffer, size);
	}
	sema_up(&syn_sema);

	return read_result;
}

int call_wait(struct res_data res_data)
{
	tid_t child_tid = res_data.rdi;
	return process_wait(child_tid);
}

int call_filesize(struct res_data res_data)
{
	int fd = res_data.rdi;

	struct file *file = find_file_by_Fd(fd);

	if (file == NULL)
	{
		return 0;
	}

	return file_length(file);
}

int call_fork(struct res_data res_data)
{
	struct thread * curr = thread_current();
	
	const char *thread_name = res_data.rdi;
	struct intr_frame * f = res_data.f;

	memcpy(&curr->ptf, f, sizeof(struct intr_frame));

	sema_down(&syn_sema);

	check_addr(thread_name);

	int result = process_fork(thread_name, &curr->ptf);
	sema_up(&syn_sema);

	return result;
	
}

int call_exec(struct res_data res_data)
{
	const char *file = res_data.rdi;
	char *file_copy;
	check_addr(file);

	file_copy = palloc_get_page(PAL_ZERO);

	if (!file_copy)
	{
		exit(-1);
		return -1;
	}

	strlcpy(file_copy, file, strlen(file) + 1);
	if (process_exec(file_copy) == -1)
	{

		exit(-1);
		return -1;
	}
}

bool call_remove(struct res_data res_data)
{
	const char *file = res_data.rdi;
	sema_down(&syn_sema);
	check_addr(file);
	bool return_ans = filesys_remove(file);
	sema_up(&syn_sema);
	return return_ans;
}

void call_seek(struct res_data res_data)
{
	int fd = res_data.rdi;
	unsigned new_pos = res_data.rsi;
	struct file *cur = thread_current()->fd_table[fd];

	if (cur != NULL)
	{
		file_seek(cur, new_pos);
	}
	
	return 0;
}

unsigned call_tell(struct res_data res_data)
{
	int fd = res_data.rdi;
	struct file *cur = thread_current()->fd_table[fd];

	if (cur != NULL)
	{
		return file_tell(cur);
	}
}

void * call_mmap(struct res_data res_data)
{
	void *addr = res_data.rdi;
	size_t length = res_data.rsi;
	int writable = res_data.rdx;
	int fd = res_data.r10;
	off_t offset = res_data.r8;

	if (fd == 0 || fd == 1 || is_kernel_vaddr(addr) || is_kernel_vaddr(addr + length) || pg_round_down(offset) != offset || pg_ofs(addr) != 0)
		return NULL;

	if (addr == 0 || length <= 0 || addr + length <= 0)
	{
		return NULL;
	}
	struct file *file = find_file_by_Fd(fd);

	if (file == NULL || offset >= file_length(file))
	{
		return NULL;
	}
	return do_mmap(addr, length, writable, file, offset);
}

void call_munmap(struct res_data res_data)
{
	void *addr = (void *) res_data.rdi;
	do_munmap(addr);
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

	// pml4_get_page는 실제 적재되어 있어야 NULL반환 안해줌.. 하지만 pml4e_walk는 실제 적재되어 있지 않고 매핑만 되어있어도 되서 pml4e walk를 쓴다
	// 유저 어드레스는 왜 체크안해도 되징? 커널 영역도 가능한것인가?.? pml4_get_page를 하려면 유저 어드레스 여야해 assertion 걸려서.. 근데 이제는 커널영역에 있는 것도
	// 막 가져오나??
	if (addr == NULL || pml4e_walk(curr->pml4, addr, false) == NULL)
	{
		sema_up(&syn_sema);
		exit(-1);
	}
}
