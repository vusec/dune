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

/* Sections info. */
extern void* __start_libdune_text;
extern void* __stop_libdune_text;
extern void* __start_libdune_rodata;
extern void* __stop_libdune_rodata;
extern void* __start_libdune_data;
extern void* __stop_libdune_data;
extern void* __start_libdune_bss;
extern void* __stop_libdune_bss;

#define LIBDUNE_TEXT_SECTION_START        ((unsigned long)&__start_libdune_text)
#define LIBDUNE_TEXT_SECTION_END          ((unsigned long)&__stop_libdune_text)
#define LIBDUNE_RODATA_SECTION_START      ((unsigned long)&__start_libdune_rodata)
#define LIBDUNE_RODATA_SECTION_END        ((unsigned long)&__stop_libdune_rodata)
#define LIBDUNE_DATA_SECTION_START        ((unsigned long)&__start_libdune_data)
#define LIBDUNE_DATA_SECTION_END          ((unsigned long)&__stop_libdune_data)
#define LIBDUNE_BSS_SECTION_START         ((unsigned long)&__start_libdune_bss)
#define LIBDUNE_BSS_SECTION_END           ((unsigned long)&__stop_libdune_bss)

#define IS_LIBDUNE_TEXT_ADDR(A) \
    ((A) >= LIBDUNE_TEXT_SECTION_START && (A) < LIBDUNE_TEXT_SECTION_END)
#define IS_LIBDUNE_RODATA_ADDR(A) \
    ((A) >= LIBDUNE_RODATA_SECTION_START && (A) < LIBDUNE_RODATA_SECTION_END)
#define IS_LIBDUNE_DATA_ADDR(A) \
    ((A) >= LIBDUNE_DATA_SECTION_START && (A) < LIBDUNE_DATA_SECTION_END)
#define IS_LIBDUNE_BSS_ADDR(A) \
    ((A) >= LIBDUNE_BSS_SECTION_START && (A) < LIBDUNE_BSS_SECTION_END)
#define IS_LIBDUNE_ADDR(A) \
    (IS_LIBDUNE_TEXT_ADDR(A) || IS_LIBDUNE_RODATA_ADDR(A) \
    || IS_LIBDUNE_DATA_ADDR(A) || IS_LIBDUNE_BSS_ADDR(A) \
    || (A) == PAGEBASE)

#endif /* __DSMMAP_H__ */

