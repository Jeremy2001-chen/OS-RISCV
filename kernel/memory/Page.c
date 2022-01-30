#include <Page.h>
#include <Driver.h>

int pageAlloc(PhysicalPage **pp) {
    printf("there's no physical page left!\n");
    *pp = NULL;
    return -1;
}

static int pageWalk(u64 *pgdir, u64 va, bool create, u64 **pte) {
    int level;
    u64 *addr = pgdir;
    for (level = 2; level >= 0; level--) {
        addr += GET_PAGE_TABLE_INDEX(va, level);
        if (!(*addr) & PTE_VALID) {
            if (create) {
                PhysicalPage *pp;
                int ret = pageAlloc(&pp);
                if (ret < 0) {
                    return ret;
                }
                (*addr) = page2pa(pp) | PTE_VALID;
                pp->ref++;
            } else {
                *pte = NULL;
                return 0;
            }
        }
        addr = PTE_ADDR(*addr);
    }
    *pte = addr + GET_PAGE_TABLE_INDEX(va, 3);
    return 0;
}

void pageInsert(u64 *pgdir, u64 va, u64 pa, u64 perm) {
    u64 *pte;
    pageWalk()
}
