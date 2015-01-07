#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "../../../libdune/dune.h"


void main()
{
    int id = syscall(SYS_getpid);
    printf("getpid: %d\n", id);
     dune_init_and_enter();
}
