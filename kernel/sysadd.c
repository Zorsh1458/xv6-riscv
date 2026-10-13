#include "types.h"
#include "riscv.h"
#include "defs.h"

int
sys_add(void)
{
    int v1, v2;
    argint(0, &v1);
    argint(1, &v2);

    return v1 + v2;
}