/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "lib/round.h"

static bool file_backed_swap_in(struct page *page, void *kva);
static bool file_backed_swap_out(struct page *page);
static void file_backed_destroy(struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void vm_file_init(void)
{
}

/* Initialize the file backed page */
bool file_backed_initializer(struct page *page, enum vm_type type, void *kva)
{
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in(struct page *page, void *kva)
{
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out(struct page *page)
{
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy(struct page *page)
{
	struct file_page *file_page = &page->file;

	if (pml4_is_dirty(thread_current()->pml4, page->va))
	{
		off_t write_bytes = file_write_at(file_page->file, page->va, file_page->file_length, file_page->offset);
	}

	file_close(file_page->file);
}

/* Do the mmap */
void *
do_mmap(void *addr, size_t length, int writable,
		struct file *file, off_t offset)
{

	// Your VM system must load pages lazily in mmap regions
	// and use the mmaped file itself as a backing store for the mapping.

	// Maps length bytes the file open as fd starting from offset byte into the process's virtual address space at addr.
	// The entire file is mapped into consecutive virtual pages starting at addr.
	// If the length of the file is not a multiple of PGSIZE,
	// then some bytes in the final mapped page "stick out" beyond the end of the file.
	// Set these bytes to zero when the page is faulted in, and discard them when the page is written back to disk.
	// If successful, this function returns the virtual address where the file is mapped.
	// On failure, it must return NULL which is not a valid address to map a file.

	uint8_t *upage;
	off_t ofs;
	uint32_t read_bytes, zero_bytes;

	// TODO: 아래 실패조건은 어떻게 추가하지...?
	// if the range of pages mapped overlaps any existing set of mapped pages,
	// including the stack or pages mapped at executable load time

	if (file_length(file) == 0 || pg_ofs(addr) != 0 || addr == 0 || length == 0)
	{
		return NULL;
	}

	// if the range of pages mapped overlaps any existing set of mapped pages, return NULL
	struct supplemental_page_table *spt = &thread_current()->spt;
	if (spt_find_page(spt, addr) != NULL)
		return NULL;

	struct file *reopened_file = file_reopen(file);
	file_seek(reopened_file, offset);

	upage = addr;
	ofs = offset;
	read_bytes = file_length(reopened_file);
	zero_bytes = (ROUND_UP(read_bytes, PGSIZE) - read_bytes);

	while (read_bytes > 0 || zero_bytes > 0)
	{
		/* Do calculate how to fill this page.
		 * We will read PAGE_READ_BYTES bytes from FILE
		 * and zero the final PAGE_ZERO_BYTES bytes. */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		/* Set up aux to pass information to the lazy_load_segment. */
		struct page_info *page_info = (struct page_info *)malloc(sizeof(struct page_info));
		page_info->file = reopened_file;
		page_info->ofs = ofs;
		page_info->upage = upage;
		page_info->read_bytes = read_bytes;
		page_info->zero_bytes = zero_bytes;
		page_info->writable = writable;

		/* Allocate page */
		if (!vm_alloc_page_with_initializer(VM_FILE, upage, writable, lazy_load_segment, page_info))
			return false;

		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		upage += PGSIZE;
		ofs += page_read_bytes;
	}

	return addr;
}

/* Do the munmap */
void do_munmap(void *addr)
{
	// 1) TODO: 수정된 파일의 dirty bit를 변경하는 작업 필요.
	// 2) TODO: 열려있는 파일이 삭제되었을 때에 대한 이해 필요. => file_reopen을 사용할 것
	// 3) TODO: 서로 다른 프로세스가 같은 파일을 바라보는 경우 두 개의 데이터가 꼭 같을 필요는 없다. 물리 페이지에 대한 two mapping을 유지함으로써 가능

	// 메모리 반환, 해제
	struct page *page;
	uint16_t pg_cnt, i;

	page = spt_find_page(&thread_current()->spt, addr);

	if (!pml4_is_dirty(thread_current()->pml4, addr)) {
		return;
	}


	if (page != NULL)
	{
		struct file_page file_page = page->file;
		// addr에 맵핑된 fd를 알아야 한다. 그리고 사이즈도 알아야 한다.
		off_t write_bytes = file_write_at(file_page.file, addr, file_page.file_length, file_page.offset);
	}
}
