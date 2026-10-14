#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"

uint64
sys_rtc_read(void)
{
	uint64 out;
	
	argaddr(0, &out);

	uint64 ns = rtc_read_ns();

	if (copyout(myproc()->pagetable, myproc()->sz, out, (char *)&ns, sizeof(ns)) < 0) {
		return -1;
	}

	return 0;
}