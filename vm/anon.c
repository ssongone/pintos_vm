/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "bitmap.h"
#include "threads/mmu.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in(struct page *page, void *kva);
static bool anon_swap_out(struct page *page);
static void anon_destroy(struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

static struct bitmap *swap_bitmap;
/* Initialize the data for anonymous pages */
void vm_anon_init(void)
{
	/* TODO: Set up the swap_disk. */

	swap_disk = disk_get(1,1);
	swap_bitmap = bitmap_create(disk_size(swap_disk));
}

/* Initialize the file mapping */
bool anon_initializer(struct page *page, enum vm_type type, void *kva)
{
	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
	size_t bit_no = anon_page->disk_sec;

	for (int i = 0 ; i < SECTOR_PER_DISK ; i++) {
		disk_read (swap_disk, bit_no*SECTOR_PER_DISK+i, kva+(i * DISK_SECTOR_SIZE));
	}
	bitmap_flip(swap_bitmap, bit_no);
	anon_page->disk_sec = NULL;
	return true;
	return true;
}


static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	
	size_t bit_idx = bitmap_scan(swap_bitmap, 0, 1, false);
	bitmap_flip(swap_bitmap, bit_idx);

	void* tmp = page->frame->kva;
	for (int i = 0 ; i < SECTOR_PER_DISK ; i++) {
		disk_write (swap_disk, bit_idx*SECTOR_PER_DISK+i, page->va+(i * DISK_SECTOR_SIZE));
	}
	pml4_clear_page(thread_current()->pml4, page->va);
	page->frame = NULL;
	anon_page->disk_sec = bit_idx;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy(struct page *page)
{
	struct anon_page *anon_page = &page->anon;

	struct frame *f = page->frame;
	if (f != NULL)
	{
		pml4_clear_page(thread_current()->pml4, page->va);
		list_remove(&f->list_elem);
		palloc_free_page(f->kva);
		free(f);
	}
}
