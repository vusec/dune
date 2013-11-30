#include <stdio.h>
#include <errno.h>

#include <libdune/dsmmap.h>

dsmmap_page_t dsmmap_pages[DSMMAP_MAX_PAGES];
unsigned long dsmmap_num_pages;

dsmmap_page_t dsmmap_iter_pages[DSMMAP_MAX_PAGES];
unsigned long dsmmap_num_iter_pages;

char dsmmap_mem[DSMMAP_MAX_PAGES*PGSIZE];
unsigned long dsmmap_num_mem_pages;

typedef struct dsmmap_cb_args_s {
    unsigned long start;
    unsigned long end;
    struct {
        unsigned long start;
        unsigned long end;
    } ranges[DSMMAP_MAX_RANGES];
    unsigned long range_idx;
} dsmmap_cb_args_t;
__thread dsmmap_cb_args_t dsmmap_cb_args;

dsmmap_stats_t dsmmap_stats;

int dsmmap_debug = DSMMAP_DEBUG_DEFAULT;
printf_t dsmmap_printf = DSMMAP_PRINTF_DEFAULT;

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
    dsmmap_printf("unhandled page fault 0x%016lx %lx\n",
        addr, tf->err);
    dune_dump_trap_frame(tf);
    dune_procmap_dump();
    dune_die();
}

static inline void dssmap_wp_pgflt_handler(uintptr_t addr, uint64_t fec, struct dune_tf *tf,
    ptent_t *pte)
{
    dsmmap_page_t page;

    dsmmap_debug_printf("[ckpt=%lu] Faulting @address 0x%016lx\n", DSMMAP_STAT(num_checkpoints), addr);

    page.addr = addr;
    dsmmap_page_list_add(&page);
    *pte &= ~PTE_USR1;
    *pte |= PTE_W;
    dune_flush_tlb_one(addr);
    DSMMAP_STAT(num_cows)++;
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

    dsmmap_page_list_clear_and_iter(&iter);
    while (dsmmap_page_list_iter_next(&page, &iter)) {
        ret = dune_vm_lookup(pgroot, (void *) page.addr, 0, &pte);
        assert(ret == 0);

        *pte &= ~PTE_W;
        *pte |= PTE_USR1;
        dune_flush_tlb_one(page.addr);
    }
    dsmmap_mem_flush();
    DSMMAP_STAT(num_checkpoints)++;
}

/* Public interface. */
int dsmmap_init()
{
    int ret;

    /* XXX: Handle fork(). */
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

static int dsmmap_procmap_entry_intersects(const struct dune_procmap_entry *ent,
    unsigned long *start, unsigned long *end)
{
    if (ent->begin >= *end || ent->end <= *start) {
        return 0;
    }
    *start = MAX(ent->begin, *start);
    *end = MIN(ent->end, *end);

    return 1;
}

static int dsmmap_procmap_entry_is_cowable(const struct dune_procmap_entry *ent)
{
    return ent->w
        && !IS_LIBDUNE_ADDR(ent->begin) && !IS_LIBDUNE_ADDR(ent->end - 1)
        && ent->type != PROCMAP_TYPE_STACK;
}

static void dsmmap_cb_args_ranges_mprotect(unsigned long perm)
{
    unsigned long len = dsmmap_cb_args.range_idx;
    int i;

    for (i=0;i<len;i++) {
        unsigned long start = dsmmap_cb_args.ranges[i].start;
        unsigned long end = dsmmap_cb_args.ranges[i].end;
        dune_vm_mprotect(pgroot, (void*)start, end-start, perm);
    }
}

static void dsmmap_cb(const struct dune_procmap_entry *ent)
{
    unsigned long start, end;

    if (!dsmmap_procmap_entry_is_cowable(ent)) {
        return;
    }
    start = dsmmap_cb_args.start;
    end = dsmmap_cb_args.end;
    if (!dsmmap_procmap_entry_intersects(ent, &start, &end)) {
        return;
    }

    dsmmap_debug_printf("dsmmap: +dune_vm_mprotect [0x%08x, 0x%08x)\n", start, end);

    assert(dsmmap_cb_args.range_idx < DSMMAP_MAX_RANGES);
    dsmmap_cb_args.ranges[dsmmap_cb_args.range_idx].start = start;
    dsmmap_cb_args.ranges[dsmmap_cb_args.range_idx++].end = end;
}

static void dsmunmap_cb(const struct dune_procmap_entry *ent)
{
    unsigned long start, end;

    if (!dsmmap_procmap_entry_is_cowable(ent)) {
        return;
    }
    start = dsmmap_cb_args.start;
    end = dsmmap_cb_args.end;
    if (!dsmmap_procmap_entry_intersects(ent, &start, &end)) {
        return;
    }

    dsmmap_debug_printf("dsmmap: -dune_vm_mprotect [0x%08x, 0x%08x)\n", start, end);

    assert(dsmmap_cb_args.range_idx < DSMMAP_MAX_RANGES);
    dsmmap_cb_args.ranges[dsmmap_cb_args.range_idx].start = start;
    dsmmap_cb_args.ranges[dsmmap_cb_args.range_idx++].end = end;
}

int dsmmap(void *addr, size_t size)
{
    /* XXX: Save dsmmapped ranges and handle brk(), mmap(), mprotect(), etc. */
    dsmmap_cb_args.range_idx = 0;
    dsmmap_cb_args.start = (unsigned long) addr;
    dsmmap_cb_args.end = dsmmap_cb_args.start + size;
    dune_procmap_iterate(&dsmmap_cb);
    dsmmap_cb_args_ranges_mprotect(PERM_U|PERM_R|PERM_USR1);

    return 0;
}

int dsmunmap(void *addr, size_t size)
{
    dsmmap_cb_args.range_idx = 0;
    dsmmap_cb_args.start = (unsigned long) addr;
    dsmmap_cb_args.end = dsmmap_cb_args.start + size;
    dune_procmap_iterate(&dsmunmap_cb);
    dsmmap_cb_args_ranges_mprotect(PERM_U|PERM_R|PERM_USR1);

    return 0;
}

