#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "ptflags.h"

static void
print_pte_flags(uint64 pte)
{
	printk("%c", (pte & PTE_R) ? 'R' : '_');
	printk("%c", (pte & PTE_W) ? 'W' : '_');
	printk("%c", (pte & PTE_X) ? 'X' : '_');
	printk("%c", (pte & PTE_U) ? 'U' : '_');
	printk("%c", (pte & PTE_G) ? 'G' : '_');
	printk("%c", (pte & PTE_A) ? 'A' : '_');
	printk("%c", (pte & PTE_D) ? 'D' : '_');
}

static void
print_hex_padded(uint64 value)
{
	static const char digits[] = "0123456789ABCDEF";

	printk("0x");
	for (int i = 2; i >= 0; i--) {
		int nibble = (value >> (i * 4)) & 0xf;
		printk("%c", digits[nibble]);
	}
}

static void
print_pte(uint64 idx, uint64 pte, int tabs)
{
	for (int i = 0; i < tabs; i++) {
		printk("         ");
    }

    print_hex_padded(idx);
	printk(" -> %p ", (void *)PTE2PA(pte));
	print_pte_flags(pte);
	printk("\n");
}

static uint64 *
walk_next(pagetable_t table, uint64 idx)
{
	uint64 pte = table[idx];
	if ((pte & PTE_V) == 0) {
		return 0;
    }
	if (pte & (PTE_R | PTE_W | PTE_X)) {
		return 0;
    }
	return (uint64 *)PTE2PA(pte);
}

uint64
sys_check_accessed_dirty(void)
{
	uint64 buf;
	int len;
	int flags;
	struct proc *p = myproc();

	argaddr(0, &buf);
	argint(1, &len);
	argint(2, &flags);

	if (len < 0) {
		return -1;
    }

	if (flags & ~PT_CLEAR_MASK) {
		return -1;
    }

	if (len == 0) {
		return 0;
    }

	uint64 end = buf + (uint64)len;
	
    if (end < buf) {
		return -2;
    }

	if (end > p->sz) {
		return -2;
    }

	uint64 va = PGROUNDDOWN(buf);
	uint64 last = PGROUNDDOWN(end - 1);

	while(1) {
		pte_t *pte = walk(p->pagetable, va, 0);

		if (pte != 0 && (*pte & PTE_V) && (*pte & PTE_U)) {
			if ((flags & PT_CLEAR_A) && (*pte & PTE_A)) {
				return 1;
			}
			if ((flags & PT_CLEAR_D) && (*pte & PTE_D)) {
				return 1;
			}
		}

		if (va == last) {
			break;
        }
		va += PGSIZE;
	}

	return 0;
}

uint64
sys_clear_accessed_dirty(void)
{
	uint64 buf;
	int len;
	int flags;
	struct proc *p = myproc();

	argaddr(0, &buf);
	argint(1, &len);
	argint(2, &flags);

	if (len < 0) {
		return -1;
    }

	if (flags & ~PT_CLEAR_MASK) {
		return -1;
    }

	if (len == 0) {
		return 0;
    }

	uint64 end = buf + (uint64)len;
	
    if (end < buf) {
		return -2;
    }

	if (end > p->sz) {
		return -2;
    }

	uint64 va = PGROUNDDOWN(buf);
	uint64 last = PGROUNDDOWN(end - 1);

	int changed = 0;
	while(1) {
		pte_t *pte = walk(p->pagetable, va, 0);

		if (pte != 0 && (*pte & PTE_V) && (*pte & PTE_U)) {
			if ((flags & PT_CLEAR_A) && (*pte & PTE_A)) {
				*pte &= ~PTE_A;
				changed = 1;
			}
			if ((flags & PT_CLEAR_D) && (*pte & PTE_D)) {
				*pte &= ~PTE_D;
				changed = 1;
			}
		}

		if (va == last) {
			break;
        }
		va += PGSIZE;
	}

	if (changed) {
		sfence_vma();
    }

	return 0;
}

uint64
sys_dump_pagetable(void)
{
	struct proc *p = myproc();
	pagetable_t root = p->pagetable;

	printk("PAGETABLE %p\n", (void *)root);

	for (uint64 i = 0; i < 512; i++) {
		uint64 pte = root[i];

		if ((pte & PTE_V) == 0) {
			continue;
        }

		print_pte(i, pte, 0);

		if (pte & (PTE_R | PTE_W | PTE_X)) {
			continue;
        }

		uint64 *mid = walk_next(root, i);
		if (mid == 0) {
			continue;
        }

		for (uint64 j = 0; j < 512; j++) {
			uint64 mpte = mid[j];

			if ((mpte & PTE_V) == 0) {
				continue;
            }

			print_pte(j, mpte, 1);

			if (mpte & (PTE_R | PTE_W | PTE_X)) {
				continue;
            }

			uint64 *leaf = walk_next(mid, j);
			if (leaf == 0) {
				continue;
            }

			for (uint64 k = 0; k < 512; k++) {
				uint64 lpte = leaf[k];

				if ((lpte & PTE_V) == 0) {
					continue;
                }

				print_pte(k, lpte, 2);
			}
		}
	}

	return 0;
}