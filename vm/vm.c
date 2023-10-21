/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/vaddr.h"

struct list frame_list;

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void vm_init(void)
{
	vm_anon_init();
	vm_file_init();
#ifdef EFILESYS /* For project 4 */
	pagecache_init();
#endif
	register_inspect_intr();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
	list_init(&frame_list);
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type(struct page *page)
{
	int ty = VM_TYPE(page->operations->type);
	switch (ty)
	{
	case VM_UNINIT:
		return VM_TYPE(page->uninit.type);
	default:
		return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim(void);
static bool vm_do_claim_page(struct page *page);
static struct frame *vm_evict_frame(void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */

typedef bool (*page_initializer)(struct page *, enum vm_type, void *kva);

bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable,
									vm_initializer *init, void *aux)
{

	ASSERT(VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current()->spt;
	// struct page *new_page = calloc(1, sizeof(struct page));

	if (spt_find_page(spt, upage) == NULL)
	{
		/* TODO: Create the page, fetch the initialier according to the VM type, */
		struct page *page = (struct page *)malloc(sizeof(struct page));
		if (!page)
			return false;

		/* TODO: and then create "uninit" page struct by calling uninit_new. */

		if (type == VM_ANON)
		{
			uninit_new(page, upage, init, type, aux, anon_initializer);
		}
		else if (type == VM_FILE)
		{
			uninit_new(page, upage, init, type, aux, file_backed_initializer);
		}


		page->writable = writable;

		/* TODO: Insert the page into the spt. */
		if (!spt_insert_page(spt, page))
		{
			goto err;
		}
	}

	return true;
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */

struct page *spt_find_page(struct supplemental_page_table *spt, void *va)
{
	/* TODO: supplemental_page_table에서 인자로 주어진 va에 해당되는 page를 찾아서 리턴하기 */

	// struct page *page = (struct page*) malloc(sizeof(struct page));
	struct page page;
	page.va = pg_round_down(va);

	struct hash_elem *now_elem = hash_find(&spt->hash_table, &page.spt_elem);

	/* TODO: Fill this function. */
	if (now_elem == NULL)
	{
		return NULL;
	}

	return hash_entry(now_elem, struct page, spt_elem);
}

/* Insert PAGE into spt with validation. */
bool spt_insert_page(struct supplemental_page_table *spt,
					 struct page *page)
{
	int succ = false;
	/* TODO: Fill this function. */

	struct hash_elem *exit_elem = hash_insert(&spt->hash_table, &page->spt_elem);

	if (exit_elem == NULL)
	{
		succ = true;
	}

	return succ;
}

void spt_remove_page(struct supplemental_page_table *spt, struct page *page)
{
	vm_dealloc_page(page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim(void)
{
	struct frame *victim = NULL;
	/* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame(void)
{
	struct frame *victim UNUSED = vm_get_victim();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() 및 get frame 사용 가능한 페이지가 없으면 해당 페이지를 퇴거하고 반환합니다.
* 이것은 항상 유효한 주소를 반환한다. 즉, 사용자 풀 메모리가 꽉 차 있으면, 
* 이 함수는 사용 가능한 메모리 공간을 얻기 위해 프레임을 제거한다. */

/*
1. 당신의 페이지 재배치 알고리즘을 이용하여, 쫓아낼 프레임을 고릅니다. 아래에서 설명할 “accessed”, “dirty” 비트들(페이지 테이블에 있는)이 유용할 것입니다.
2. 해당 프레임을 참조하는 모든 페이지 테이블에서 참조를 제거합니다. 공유를 구현하지 않았을 경우, 해당 프레임을 참조하는 페이지는 항상 한 개만 존재해야 합니다.
3. 필요하다면, 페이지를 파일 시스템이나 스왑에 write 합니다. 쫓아내어진(evicted) 프레임은 이제 다른 페이지를 저장하는 데에 사용할 수 있습니다.
*/
static struct frame *
vm_get_frame(void)
{
	struct frame *frame = NULL;
	void *addr = palloc_get_page(PAL_USER | PAL_ZERO);

	if (addr == NULL) {
		// printf("공간이 없어..!\n");
		struct list_elem *out_elem = list_pop_front(&frame_list);
		struct frame* out_frame = list_entry(out_elem, struct frame, list_elem);
		// out_frame을 일단 디스크로 보내야해..
		swap_out(out_frame->page);

		frame = out_frame;
		frame->page = NULL;
	} else {
		frame = calloc(1, sizeof(struct frame));
		frame->kva = addr;
	}
	list_push_back(&frame_list, &frame->list_elem);

	ASSERT(frame != NULL);
	ASSERT(frame->page == NULL);

	return frame;
}

/* Growing the stack. */

static bool
vm_stack_growth(void *addr)
{
	// printf("stack_growth()\n");
	struct thread *curr = thread_current();
	void *before_buttom = curr->stack_bottom;

	// if (addr < before_buttom && curr->tf.rsp > before_buttom) {
	// 	call_exit(curr, -1);
	// }
	// void * check_empty = curr->tf.rsp;
	// while (check_empty <= before_buttom) {
	// 	//빈놈이 있으면 안돼?.?
	// 	if (check_empty == NULL) {
	// 		call_exit(curr, -1);
	// 	}
	// 	check_empty += sizeof(void *);
	// }

	curr->stack_bottom -= PGSIZE;

	// printf("stack_bottom 작아졌니? %d\n", thread_current()->stack_bottom);
	// void *before_buttom = (void *)(((uint8_t *)USER_STACK) - PGSIZE);
	curr->tf.rsp = before_buttom;

	// printf("zzz\n");
	vm_alloc_page(VM_ANON, curr->stack_bottom, true);
	// printf("xxx\n");
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp(struct page *page UNUSED)
{
}

/*
* user : true - 유저모드, false - 커널모드
* not-present : 해당 인자가 false인 경우는 read-only 페이지에 write를 하려는 상황을 나타냄. 주어진 테스트 케이스에서는 mmap-ro 케이스가 해당 인자를 체크함

/* Return true on success */
bool vm_try_handle_fault(struct intr_frame *f, void *addr, bool user, bool write, bool not_present)
{
	/*!SECTION
	 you load some contents into the page and return control to the user program
	*/
	struct thread *curr = thread_current();
	struct supplemental_page_table *spt = &thread_current()->spt;
	struct page *page = NULL;

	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	// 구현에 도움을 드리자면 우선 인자로 들어오면 addr의 유효성을 검증하고,뒤이어 현재 쓰레드의 rsp_stack를 받아오거나 인터럽트 프레임의 rsp를 받아와 현재 쓰레드의 rsp 주소를 설정합니다.
	if (addr == NULL || is_kernel_vaddr(addr))
	{
		call_exit(curr, -1);
	}
	// if (USER_STACK > addr && curr->tf.rsp > addr) {
	// 	call_exit(curr, -1);
	// }

	// 스택을 다썻는지 안썻는지..
	// addr이랑 f->rsp랑 비교?? spt에는 들어있지 않은 addr.....
	// 코드영역은 spt에 들어와있으니까...
	// round_up은??
	// stack growth 해야하는 경우: 그 페이지에 해당하는 spt가 없어고, 스택이 꽉차있음(= rsp가 더 작아졌어 지금 가리키는 주소보다)
	// 스택
	if (spt_find_page(spt, addr) == NULL && curr->stack_bottom > addr)
	{

		if (f->rsp != addr)
		{
			return false;
		}
		// printf("stack_bottom 은: %p\n ", thread_current()->stack_bottom);
		// printf("addr은: %p\n ", addr);
		vm_stack_growth(addr);
	}

	page = spt_find_page(spt, addr);

	if (page == NULL)
	{
		return false;
	}

	if (write && !page->writable)
	{
		call_exit(curr, -1);
	}

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
	return vm_do_claim_page(page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void vm_dealloc_page(struct page *page)
{
	destroy(page);
	free(page);
}

/* Claim the page that allocate on VA. */
bool vm_claim_page(void *va)
{
	struct page *page = NULL;
	/* TODO: Fill this function */
	va = pg_round_down(va);
	page = spt_find_page(&thread_current()->spt, va);
	if (page == NULL)
		return false;
	return vm_do_claim_page(page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page(struct page *page)
{
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

	if (!pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable))
	{
		return false;
	}
	return swap_in(page, frame->kva);
}

/* Initialize new supplemental page table */
void supplemental_page_table_init(struct supplemental_page_table *spt)
{
	hash_init(&spt->hash_table, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool supplemental_page_table_copy(struct supplemental_page_table *dst, struct supplemental_page_table *src)
{
	hash_apply(&src->hash_table, page_hash_copy);
	return true;
}

/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill(struct supplemental_page_table *spt)
{
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */

	// hash_apply(&spt->hash_table, page_kill);

	hash_destroy(spt, page_hash_destructor); // page, frame 관련 리소스 해제 / 버킷 리스트 해제
}

/* Returns a hash value for page p. */
unsigned page_hash(const struct hash_elem *p_, void *aux)
{
	const struct page *p = hash_entry(p_, struct page, spt_elem);
	return hash_bytes(&p->va, sizeof p->va);
}

bool page_less(const struct hash_elem *a_,
			   const struct hash_elem *b_, void *aux)
{
	const struct page *a = hash_entry(a_, struct page, spt_elem);
	const struct page *b = hash_entry(b_, struct page, spt_elem);

	return a->va < b->va;
}

void page_hash_destructor(struct hash_elem *e, void *aux)
{
	struct page *page = hash_entry(e, struct page, spt_elem);
	vm_dealloc_page(page);

}

void page_hash_copy(struct hash_elem *src_elem, void *aux)
{
	struct page *src_p = hash_entry(src_elem, struct page, spt_elem);

	if (src_p->operations->type == VM_UNINIT)
	{
		vm_alloc_page_with_initializer(VM_ANON, src_p->va, src_p->writable, src_p->uninit.init, src_p->uninit.aux);
	}
	else
	{
		vm_alloc_page(VM_ANON, src_p->va, src_p->writable);
		vm_claim_page(src_p->va);
		struct page *child_page = spt_find_page(&thread_current()->spt, src_p->va);
		if (src_p->frame != NULL)
			memcpy(child_page->frame->kva, src_p->frame->kva, PGSIZE);
	}
}

void page_kill(struct hash_elem *elem, void *aux)
{
	struct page *page = hash_entry(elem, struct page, spt_elem);

	// VM_FILE
	// if (page->operations->type == VM_FILE)
	// {
	// 	do_munmap(page->va);
	// }

	struct frame *f = page->frame;
	if (f != NULL) {
		pml4_clear_page(thread_current()->pml4, page->va);
		list_remove(&f->list_elem);
		palloc_free_page(f->kva);
		free(f);
	}

}