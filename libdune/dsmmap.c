#include <stdio.h>
#include <errno.h>

#include <libdune/dsmmap.h>

dsmmap_page_t dsmmap_pages[DSMMAP_MAX_PAGES];
unsigned long dsmmap_num_pages = 0;

dsmmap_page_t dsmmap_iter_pages[DSMMAP_MAX_PAGES];
unsigned long dsmmap_num_iter_pages = 0;

char dsmmap_mem[DSMMAP_MAX_PAGES*PGSIZE];
unsigned long dsmmap_num_mem_pages = 0;

void dsmmap_mem_flush()
{
    dsmmap_num_mem_pages = 0;
}

void dsmmap_page_list_add(dsmmap_page_t *page)
{
    dsmmap_page_t *new_page;
    char *new_mem;

    assert(dsmmap_num_pages < DSMMAP_MAX_PAGES);
    new_page = &dsmmap_pages[dsmmap_num_pages++];
    new_page->addr = page->addr;

    assert(dsmmap_num_mem_pages < DSMMAP_MAX_PAGES);
    new_mem = &dsmmap_mem[dsmmap_num_mem_pages++];
    new_page->mem = new_mem;
    memcpy(new_page->mem, (void*)new_page->addr, PGSIZE);
}

void dsmmap_page_list_clear_and_iter(dsmmap_page_t **iter)
{
    *iter = dsmmap_iter_pages;
    dsmmap_num_iter_pages = dsmmap_num_pages;
    memcpy(dsmmap_iter_pages, dsmmap_pages, dsmmap_num_iter_pages*sizeof(dsmmap_page_t));
    dsmmap_num_pages = 0;
}

int dsmmap_page_list_iter_next(dsmmap_page_t *page, dsmmap_page_t **iter)
{
    int index = *iter - dsmmap_iter_pages;

    if (index >= dsmmap_num_iter_pages) {
        return 0;
    }

    *page = dsmmap_iter_pages[index];
    (*iter)++;
    return 1;
}

void dsmmap_default_pgflt_handler(uintptr_t addr, uint64_t fec, struct dune_tf *tf)
{
    dune_printf("unhandled page fault 0x%016lx %lx\n",
        addr, tf->err);
    dune_dump_trap_frame(tf);
    dune_procmap_dump();
    dune_die();
}

static inline void dssmap_wp_pgflt_handler(uintptr_t addr, uint64_t fec, struct dune_tf *tf,
    ptent_t *pte)
{
    dsmmap_page_t page;

#if DSMMAP_DEBUG
    dune_printf("Faulting @address 0x%016lx\n", addr);
#endif

    page.addr = addr;
    dsmmap_page_list_add(&page);
    *pte &= ~PTE_USR1;
    *pte |= PTE_W;
    dune_flush_tlb_one(addr);
}

void dsmmap_pgflt_handler(uintptr_t addr, uint64_t fec, struct dune_tf *tf)
{
    ptent_t *pte = NULL;
    int rc;

    if (!(fec & FEC_W)) {
        dsmmap_default_pgflt_handler(addr, fec, tf);
        return;
    }

    rc = dune_vm_lookup(pgroot, (void *) addr, 0, &pte);
    assert(rc == 0);

    if (!((*pte) & PTE_USR1)) {
        dsmmap_default_pgflt_handler(addr, fec, tf);
        return;
    }

    dssmap_wp_pgflt_handler(addr, fec, tf, pte);
}

static void dsmctl_checkpoint()
{
    dsmmap_page_t page;
    dsmmap_page_t *iter;
    ptent_t *pte = NULL;
    int ret;

#if DSMMAP_DEBUG
    dune_printf("Checkpointing dirty pages...\n");
#endif

    dsmmap_page_list_clear_and_iter(&iter);
    while (dsmmap_page_list_iter_next(&page, &iter)) {
        ret = dune_vm_lookup(pgroot, (void *) page.addr, 0, &pte);
        assert(ret == 0);

        *pte &= ~PTE_W;
        *pte |= PTE_USR1;
        dune_flush_tlb_one(page.addr);
    }
    dsmmap_mem_flush();
}

int dsmmap_init()
{
    int ret;

    ret = dune_init_and_enter();
    if (ret) {
        printf("failed to initialize dune\n");
        return ret;
    }
    dune_register_pgflt_handler(dsmmap_pgflt_handler);

    return 0;
}

int dsmctl(dsmmap_dsmctl_op_t op, void *ptr)
{
    int ret;

    (void)(ptr);
    switch(op) {
    case DSMMAP_DSMCTL_CHECKPOINT:
        dsmctl_checkpoint();
        ret = 0;
        break;
    default:
        ret = EINVAL;
        break;
    }

    return ret;
}

int dsmmap(void *addr, size_t size)
{
    dune_vm_mprotect(pgroot, addr, size, PERM_U|PERM_R|PERM_USR1);
    return 0;
}

int dsmunmap(void *addr, size_t size)
{
    dune_vm_mprotect(pgroot, addr, size, PERM_U|PERM_R|PERM_W);
    return 0;
}

