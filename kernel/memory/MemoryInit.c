#include <Page.h>
#include <Driver.h>

PageList freePages;
PhysicalPage pages[PHYSICAL_PAGE_NUM];

static void initFreePages() {
    extern char kernelStart[];
    extern char kernelEnd[];
    u32 i;
    u32 n = PA2PPN(KERNEL_STACK_TOP);
    for (i = 0; i < n; i++) {
        pages[i].ref = 1;
    }
    
    n = PA2PPN(kernelStart);
    for (; i < n; i++) {
        pages[i].ref = 0;
        LIST_INSERT_HEAD(&freePages, &pages[i], link);
    }

    n = PA2PPN(kernelEnd);
    for (; i < n; i++) {
        pages[i].ref = 1;
    }

    n = PA2PPN(PHYSICAL_MEMORY_TOP);
    for (; i < n; i++) {
        pages[i].ref = 0;
        LIST_INSERT_HEAD(&freePages, &pages[i], link);
    }
}

static void virtualMemory() {
    extern u64 kernelPageDirectory[];
    pageInsert(kernelPageDirectory, 0, )
}

void memoryInit() {
    printf("memory init start\n");
    initFreePages();
    virtualMemory();
    printf("memory init finish\n");
}

void bcopy(void *src, void *dst, u32 len) {
    void *finish = src + len;

    if (len <= 7) {
        while (src < finish) {
            *(u8*)src = *(u8*)dst;
            src++;
            dst++;
        }
        return;
    }
    while (((u64)src) & 7) {
        *(u8*)src = *(u8*)dst;
        src++;
        dst++;
    }
    while (src + 7 < finish) {
        *(u64*)src = *(u64*)dst;
        src += 8;
        dst += 8;
    }
    while (src < finish){
        *(u8*)src = *(u8*)dst;
        src++;
        dst++;
    }
}

void bzero(void *start, u32 len) {
    void *finish = start + len;

    if (len <= 7) {
        while (start < finish) {
            *(u8*)start++ = 0;
        }
        return;
    }
    while (((u64) start) & 7) {
        *(u8*)start++ = 0;
    }
    while (start + 7 < finish) {
        *(u64*)start = 0;
        start += 8;
    }
    while (start < finish) {
        *(u8*)start++ = 0;
    }
}