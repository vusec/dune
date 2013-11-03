#ifndef __DSMMAP_H__
#define __DSMMAP_H__

#include <libdune/dune.h>
#include <libdune/cpu-x86.h>

#define DSMMAP_DEBUG            1
#define DSMMAP_MAX_PAGES     1000
#define DSMMAP_MAX_RANGES     100

#define MIN(X, Y) (X < Y ? X : Y)
#define MAX(X, Y) (X > Y ? X : Y)

typedef enum dsmmap_dsmctl_op_e {
    DSMMAP_DSMCTL_CHECKPOINT,
    __NUM_DSMMAP_DSMCTL_OPS
} dsmmap_dsmctl_op_t;

typedef struct dsmmap_page_s {
    uintptr_t addr;
    void *mem;
} dsmmap_page_t;

int dsmmap_init();
int dsmmap(void *addr, size_t size);
int dsmunmap(void *addr, size_t size);
int dsmctl(dsmmap_dsmctl_op_t op, void *ptr);

#endif /* __DSMMAP_H__ */
