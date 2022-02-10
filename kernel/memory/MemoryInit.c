#include <Page.h>
#include <Driver.h>
#include <MemoryConfig.h>
#include <Riscv.h>

PageList freePages;
PhysicalPage pages[PHYSICAL_PAGE_NUM];
extern char kernelStart[];
extern char kernelEnd[];
extern u64 kernelPageDirectory[];

static void initFreePages() {
    u32 i;
    u32 n = PA2PPN(kernelEnd);
    for (i = 0; i < n; i++) {
        pages[i].ref = 1;
    }

    n = PA2PPN(PHYSICAL_MEMORY_TOP);
    for (; i < n; i++) {
        pages[i].ref = 0;
        LIST_INSERT_HEAD(&freePages, &pages[i], link);
    }
}

static void virtualMemory() {
    pageInsert(kernelPageDirectory, UART_V, UART, PTE_READ | PTE_WRITE);
    #ifdef QEMU
    pageInsert(kernelPageDirectory, VIRTIO0_V, VIRTIO0, PTE_READ | PTE_WRITE);
    #endif
    u64 va = CLINT_V, pa = CLINT, i;
    for (i = 0; i < 0x10000; i += PAGE_SIZE) {
        pageInsert(kernelPageDirectory, va + i, pa + i, PTE_READ | PTE_WRITE);
    }
    va = PLIC_V; pa = PLIC;
    for (i = 0; i < 0x4000; i += PAGE_SIZE) {
        pageInsert(kernelPageDirectory, va + i, pa + i, PTE_READ | PTE_WRITE);
    }
    va = PLIC_V + 0x200000; pa = PLIC + 0x200000;
    for (i = 0; i < 0x4000; i += PAGE_SIZE) {
        pageInsert(kernelPageDirectory, va + i, pa + i, PTE_READ | PTE_WRITE);
    }
    #ifndef QEMU
    // to do
    #endif
    
    extern char textEnd[];
    va = pa = (u64)kernelStart;
    for (i = 0; va + i < (u64)textEnd; i += PAGE_SIZE) {
        pageInsert(kernelPageDirectory, va + i, pa + i, PTE_READ | PTE_EXECUTE | PTE_WRITE);
    }
    va = pa = (u64)textEnd;
    for (i = 0; va + i < PHYSICAL_MEMORY_TOP; i += PAGE_SIZE) {
        pageInsert(kernelPageDirectory, va + i, pa + i, PTE_READ | PTE_WRITE);
    }
    extern char trampoline[];
    pageInsert(kernelPageDirectory, TRAMPOLINE_BASE, (u64)trampoline, 
        PTE_READ | PTE_WRITE | PTE_EXECUTE);
}

static void startPage() {
    w_satp(MAKE_SATP(kernelPageDirectory));
    sfence_vma();
}

static void testMemory() {
    PhysicalPage *p;
    pageAlloc(&p);
    printf("alloced page1:  %lx\n", page2pa(p));
    pageInsert(kernelPageDirectory, 1ll << 35, page2pa(p), PTE_READ | PTE_WRITE);
    *((u32*)(1ll<<35)) = 147893;
    printf("value1 of %lx:  %d\n", 1ll << 35, *((u32*)(1ll<<35)));
    pageAlloc(&p);
    printf("alloced page2:  %lx\n", page2pa(p));
    pageInsert(kernelPageDirectory, 1ll << 35, page2pa(p), PTE_READ | PTE_WRITE);
    *((u32*)(1ll<<35)) = 609475;
    printf("value1 of %lx:  %d\n", 1ll << 35, *((u32*)(1ll<<35)));
}

void memoryInit() {
    initFreePages();
    virtualMemory();
    startPage();
    printf("memory init finish\n");
    testMemory();
}

void bcopy(void *src, void *dst, u32 len) {
    void *finish = src + len;

    if (len <= 7) {
        while (src < finish) {
            *(u8*)dst = *(u8*)src;
            src++;
            dst++;
        }
        return;
    }
    while (((u64)src) & 7) {
        *(u8*)dst = *(u8*)src;
        src++;
        dst++;
    }
    while (src + 7 < finish) {
        *(u64*)dst = *(u64*)src;
        src += 8;
        dst += 8;
    }
    while (src < finish){
        *(u8*)dst = *(u8*)src;
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