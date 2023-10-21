/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "lib/round.h"
#include "threads/mmu.h"

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
	struct file_page *file_page = &page->file;
	
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
		off_t write_bytes = file_write_at(file_page->file, page->va, file_page->read_bytes, file_page->offset);
	}

	// do_munmap(page->va);

	// pml4_clear_page(thread_current()->pml4, page->va);
	// struct frame *f = page->frame;
	// if (f != NULL)
	// {
	// 	// printf("🎁: %p\n", f->kva);
	// 	list_remove(&f->list_elem);
	// 	// palloc_free_page(f->kva);
	// 	free(f);
	// }
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
	int pg_count;
	// TODO: 아래 실패조건은 어떻게 추가하지...?
	// if the range of pages mapped overlaps any existing set of mapped pages,
	// including the stack or pages mapped at executable load time

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
	pg_count = (length / PGSIZE);

	while (read_bytes > 0 || zero_bytes > 0)
	{
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		struct page_info *page_info = (struct page_info *)malloc(sizeof(struct page_info));
		page_info->file = reopened_file;
		page_info->ofs = ofs;
		page_info->upage = upage;
		page_info->read_bytes = page_read_bytes;
		page_info->zero_bytes = page_zero_bytes;
		page_info->writable = writable;
		page_info->page_count = pg_count;

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
	struct page *page;
	page = spt_find_page(&thread_current()->spt, addr);

	struct file *temp_file = page->file.file;
	int page_count = page->file.count;

	while (page_count > 0)
	{
		page = spt_find_page(&thread_current()->spt, addr);
		if (pml4_is_dirty(thread_current()->pml4, addr))
		{
			if (page != NULL)
			{
				struct file_page file_page = page->file;
				off_t write_bytes = file_write_at(file_page.file, addr, file_page.read_bytes, file_page.offset);
			}
		}

		pml4_clear_page(thread_current()->pml4, addr);
		struct frame *f = page->frame;
		// if (f != NULL)
		// {
		// printf("🎁: %p\n", f->kva);
		list_remove(&f->list_elem);
		palloc_free_page(f->kva);
		hash_delete(&thread_current()->spt.hash_table, &page->spt_elem);
		free(f);
		// }

		addr += PGSIZE;
		page_count--;
	}

	file_close(temp_file);
}
