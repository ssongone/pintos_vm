#ifndef VM_ANON_H
#define VM_ANON_H
#include "vm/vm.h"
#include "threads/vaddr.h"
#include "devices/disk.h"

struct page;
enum vm_type;

struct anon_page {
    // 스왑디스크 어디에 있는지...
    disk_sector_t disk_sec;
};

void vm_anon_init (void);
bool anon_initializer (struct page *page, enum vm_type type, void *kva);

#endif
