#ifndef _PAGE_H_
#define _PAGE_H_

#include <Type.h>
#include <Queue.h>
#include <MemoryManage.h>

struct PhysicalPage;
LIST_HEAD(PageList, PhysicalPage);
typedef struct PageList PageList;
typedef LIST_ENTRY(PhysicalPage) PageListEntry;

typedef struct PhysicalPage {
    PageListEntry link;
    u32 ref;
} PhysicalPage;

inline u32 page2PPN(PhysicalPage *page) {
    extern PhysicalPage pages[];
    return page - pages;
}

inline u64 page2pa(PhysicalPage *page) {
    return page2PPN(page) << PAGE_SHIFT;
}

void bcopy(void *src, void *dst, u32 len);
void bzero(void *start, u32 len);
void memoryInit(void);

#define PA2PPN(va) ((((u64)(va)) & 0x0fffffff) >> PAGE_SHIFT)
#define PPN2PA(ppn) (((u64)(ppn)) << PAGE_SHIFT)
#define GET_PAGE_TABLE_INDEX(va, level) (((va) >> (PAGE_SHIFT + 9 * level)) & 0x1ff)

#define PTE_VALID (1ll << 0)
#define PTE_READ (1ll << 1)
#define PTE_WRITE (1ll << 2)
#define PTE_EXECUTE (1ll << 3)
#define PTE_USER (1ll << 4)
#define PERM_WIDTH 10
#define PTE_ADDR(pte) ((((u64)(pte)) >> PERM_WIDTH) << PAGE_SHIFT)

PhysicalPage* pageAlloc();
void pageInsert(u64 *pgdir, u64 va, u64 pa, u64 perm);

#endif