#include <Page.h>
#include <Driver.h>
#include <Error.h>

extern PageList freePages;
int pageAlloc(PhysicalPage **pp) {
    PhysicalPage *page;
    if ((page = LIST_FIRST(&freePages)) != NULL) {
        *pp = page;
        LIST_REMOVE(page, link);
        bzero((void*)page2pa(page), PAGE_SIZE);
        return 0;
    }
    printf("there's no physical page left!\n");
    *pp = NULL;
    return -NO_FREE_MEMORY;
}

static int pageWalk(u64 *pgdir, u64 va, bool create, u64 **pte) {
    int level;
    u64 *addr = pgdir;
    for (level = 2; level > 0; level--) {
        addr += GET_PAGE_TABLE_INDEX(va, level);
        if (!(*addr) & PTE_VALID) {
            if (!create) {
                *pte = NULL;
                return 0;
            }
            PhysicalPage *pp;
            int ret = pageAlloc(&pp);
            if (ret < 0) {
                return ret;
            }
            (*addr) = page2pte(pp) | PTE_VALID;
            pp->ref++;
        }
        addr = (u64*)PTE2PA(*addr);
    }
    *pte = addr + GET_PAGE_TABLE_INDEX(va, 0);
    return 0;
}

u64 pageLookup(u64 *pgdir, u64 va, u64 **pte) {
    u64 *entry;
    pageWalk(pgdir, va, false, &entry);

    if (entry == NULL || !(*entry & PTE_VALID)) {
        return 0;
    }
    if (pte) {
        *pte = entry;
    }
    return PTE2PA(*entry);
}

void pageFree(PhysicalPage *page) {
    if (page->ref > 0) {
        return;
    }
    if (page->ref == 0) {
        LIST_INSERT_HEAD(&freePages, page, link);
    }
}

void pageRemove(u64 *pgdir, u64 va) {
    u64 *pte;
    u64 pa = pageLookup(pgdir, va, &pte);

    // tlb flush
    if (pa < PHYSICAL_ADDRESS_BASE || pa >= PHYSICAL_MEMORY_TOP) {
        return;
    }
    PhysicalPage *page = pa2page(pa);
    page->ref--;
    pageFree(page);
    *pte = 0;
}

int pageInsert(u64 *pgdir, u64 va, u64 pa, u64 perm) {
    u64 *pte;
    va = DOWN_ALIGN(va, PAGE_SIZE);
    pa = DOWN_ALIGN(pa, PAGE_SIZE);
    int ret = pageWalk(pgdir, va, true, &pte);
    if (ret < 0) {
        return ret;
    }
    if (pte != NULL && (*pte & PTE_VALID)) {
        pageRemove(pgdir, va);
    }
    if (pa >= PHYSICAL_ADDRESS_BASE && pa < PHYSICAL_MEMORY_TOP) {
        pa2page(pa)->ref++;
    }
    *pte = PA2PTE(pa) | perm | PTE_VALID;
    return 0;
}

void pageout(u64 *pgdir, u64 badAddr) {
    if (badAddr <= PAGE_SIZE) {
        panic("^^^^^^^^^^TOO LOW^^^^^^^^^^^\n");
    }
    printf("pageout at %lx", badAddr);
    PhysicalPage *page;
    if (pageAlloc(&page) < 0) {
        panic("");
    }
    if (pageInsert(pgdir, badAddr, page2pa(page), 
        PTE_USER | PTE_READ | PTE_WRITE) < 0) {
        panic("");
    }
}