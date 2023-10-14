/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/vaddr.h"

struct hash frame_table;

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
	// hash_init(&frame_table, frame_hash, frame_less_func, NULL);

}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */

typedef bool (*page_initializer) (struct page *, enum vm_type, void *kva);

bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;
	// struct page *new_page = calloc(1, sizeof(struct page));

    if (spt_find_page (spt, upage) == NULL) {
        /* TODO: Create the page, fetch the initialier according to the VM type, */
        struct page *page = (struct page *)malloc(sizeof(struct page));
        if (!page)
            return false;
        /* TODO: and then create "uninit" page struct by calling uninit_new. */


		if (type == VM_ANON) {
			uninit_new(page, upage, init, type, aux, anon_initializer);
		} else if (type == VM_FILE) {
			uninit_new(page, upage, init, type, aux, file_backed_initializer);
		}

		page->writable = writable;
		/* TODO: Insert the page into the spt. */
		if(!spt_insert_page(spt, page)) {
			goto err;
		}

	}

	return true;
err:
	return false;
}

// typedef bool (*page_initializer) (struct page *, enum vm_type, void *kva);
// /* Create the pending page object with initializer. If you want to create a
// * page, do not create it directly and make it through this function or
// * `vm_alloc_page`.
// * 인자로 전달한 vm_type에 맞는 적절한 초기화 함수를 가져와야 하고,
// * 이 함수를 인자로 갖는 uninit_new() 함수를 호출해야 한다. */
// bool
// vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
//         vm_initializer *init, void *aux) {
//     ASSERT (VM_TYPE(type) != VM_UNINIT)
//     struct supplemental_page_table *spt = &thread_current ()->spt;
//     /* Check wheter the upage is already occupied or not. */
//     if (spt_find_page (spt, upage) == NULL) {
//         /* TODO: Create the page, fetch the initialier according to the VM type, */
//         struct page *page = (struct page *)malloc(sizeof(struct page));
//         if (!page)
//             return false;
//         /* TODO: and then create "uninit" page struct by calling uninit_new. */
//         page_initializer *new_initializer = NULL;
//         switch (type) {
//             case VM_ANON:
//                 new_initializer = anon_initializer;
//                 break;
//             case VM_FILE:
//                 new_initializer = file_backed_initializer;
//                 break;
//             default:
//                 free(page);
//                 return false;
//         }
//         /* TODO: You should modify the field after calling the uninit_new. */
//         uninit_new(page, upage, init, type, aux, new_initializer);
//         page->writable = writable;
//         /* TODO: Insert the page into the spt. */
//         spt_insert_page(spt, page);
//         return true;
//     }
// err:
//     return false;
// }







/* Find VA from spt and return page. On error, return NULL. */


/*
 송원 : va를 키로 해시함수를 하기 때문에 NULL 페이지에 va 넣어주고 hash_find() 하는 걸까?
	   va 주소에 해당하는 가상 페이지를 리턴해줘 얘 한테 정보 추가해줄거야
*/
struct page * spt_find_page (struct supplemental_page_table *spt, void *va) {
	/* TODO: supplemental_page_table에서 인자로 주어진 va에 해당되는 page를 찾아서 리턴하기 */

	// struct page *page = (struct page*) malloc(sizeof(struct page));
	struct page page;
	page.va = pg_round_down(va);

	struct hash_elem* now_elem = hash_find(&spt->hash_table, &page.spt_elem);

	/* TODO: Fill this function. */
	if (now_elem == NULL) {
		return NULL;
	}
	
	return hash_entry(now_elem, struct page, spt_elem);
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt,
		struct page *page ) {
	int succ = false;
	/* TODO: Fill this function. */

	struct hash_elem* exit_elem = hash_insert(&spt->hash_table, &page->spt_elem);

	if (exit_elem == NULL) {
		succ = true;
	}

	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	void *addr = palloc_get_page(PAL_USER | PAL_ZERO);
	frame = calloc(1, sizeof (struct frame));
	frame->kva = addr;

	/* TODO: Fill this function. */

	// hash_insert(frame_table, &frame->hash_elem);

	ASSERT(frame != NULL);
	ASSERT(frame->page == NULL);

	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool vm_try_handle_fault(struct intr_frame *f, void *addr, bool user, bool write, bool not_present)
{
	/*!SECTION
	 you load some contents into the page and return control to the user program
	*/

	struct supplemental_page_table *spt = &thread_current()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	/*TODO -
		Check if the memory reference is valid.
		- locate the content that needs to go into the virtual memory page
		- from the file, from the swap or can simply be all-zero page.
		For shared page, the page can be already in the page frame, but not in the page table
		Invalid access -> kill the process
		- Not valid user address
		- Kernel address
		- Permission error (attempt to write to the read-only page)
		Allocate page frame.
		Fetch the data from the disk to the page frame.
		Update page table.

		Most importantly, on a page fault,
		the kernel looks up the virtual page that faulted in the supplemental page table
		to find out what data should be there.
	*/

	page = spt_find_page(spt, addr);

	return vm_do_claim_page(page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va) {
	struct page *page = NULL;
	/* TODO: Fill this function */
	va = pg_round_down(va);
	page = spt_find_page(&thread_current()->spt, va);
	if (page==NULL)
		return false;
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	/*!SECTION
	Claims, meaning allocate a physical frame, a page.
	You first get a frame by calling vm_get_frame (which is already done for you in the template).
	Then, you need to set up the MMU.
	In other words, add the mapping from the virtual address to the physical address in the page table.
	The return value should indicate whether the operation was successful or not.
	*/
	
	if (!pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable)) {
		return false;
	}
	return swap_in(page, frame->kva);
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt) {
	hash_init(&spt->hash_table, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}


/* Returns a hash value for page p. */
unsigned page_hash (const struct hash_elem *p_, void *aux) {
  const struct page *p = hash_entry (p_, struct page, spt_elem);
  return hash_bytes (&p->va, sizeof p->va);
}

bool page_less (const struct hash_elem *a_,
           const struct hash_elem *b_, void *aux) {
  const struct page *a = hash_entry (a_, struct page, spt_elem);
  const struct page *b = hash_entry (b_, struct page, spt_elem);

  return a->va < b->va;
}