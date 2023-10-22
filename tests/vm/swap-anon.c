/* 익명 페이지가 스왑 아웃되고 제대로 스왑 인되었는지 확인합니다 
이 테스트에서 Pintos 메모리 크기는 10MB입니다 
* 먼저, 큰 메모리 청크를 할당하고, 각 청크에 대한 쓰기 작업을 수행합니다, 
* 그런 다음 데이터가 일치하는지 확인합니다
* 마지막으로 할당된 메모리를 해제합니다. */

#include <string.h>
#include <stdint.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"


#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define ONE_MB (1 << 20) // 1MB
#define CHUNK_SIZE (20*ONE_MB)
#define PAGE_COUNT (CHUNK_SIZE / PAGE_SIZE)

static char big_chunks[CHUNK_SIZE];

void
test_main (void) 
{
	size_t i;
    void* pa;
    char *mem;

    for (i = 0 ; i < PAGE_COUNT ; i++) {
        if(!(i % 512))
            msg ("write sparsely over page %zu", i);
        mem = (big_chunks+(i*PAGE_SIZE));
        *mem = (char)i;
    }

    for (i = 0 ; i < PAGE_COUNT ; i++) {
        mem = (big_chunks+(i*PAGE_SIZE));
        if((char)i != *mem) {
		    fail ("data is inconsistent");
        }
        if(!(i % 512))
            msg ("check consistency in page %zu", i);
    }
}

