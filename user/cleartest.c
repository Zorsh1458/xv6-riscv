#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(void)
{
    printf("INITIAL PAGETABLE --------------------------\n");
    dump_pagetable();

	char *buf = malloc(3 * 4096);
	if(buf == 0){
		printf("malloc failed\n");
		exit(1);
	}

	for (int i = 0; i < 3 * 4096; i++) {
		buf[i] = (char)(i & 0xff);
    }

    printf("\n\nPAGETABLE AFTER ALLOCATION -------------\n");
    dump_pagetable();

	int r = clear_accessed_dirty(buf, 100, 0x4);
	printf("[TEST] flags=0x4 -> %d (expect -1)\n", r);

	r = clear_accessed_dirty(buf, -1, PT_CLEAR_A);
	printf("[TEST] len=-1 -> %d (expect -1)\n", r);

	r = clear_accessed_dirty((void *)0xffffffffffff0000ULL, 16, PT_CLEAR_A);
	printf("[TEST] bad ptr -> %d (expect -2)\n", r);
	
    r = check_accessed_dirty(buf, 3 * 4096, PT_CLEAR_A);
	printf("[TEST] Check A -> %d (expect 1)\n", r);
    r = check_accessed_dirty(buf, 3 * 4096, PT_CLEAR_D);
	printf("[TEST] Check D -> %d (expect 1)\n", r);

	r = clear_accessed_dirty(buf, 3 * 4096, PT_CLEAR_A);
	printf("[TEST] valid clear A -> %d (expect 0)\n", r);
    r = check_accessed_dirty(buf, 3 * 4096, PT_CLEAR_A);
	printf("[TEST] Check A -> %d (expect 0)\n", r);
    r = check_accessed_dirty(buf, 3 * 4096, PT_CLEAR_D);
	printf("[TEST] Check D -> %d (expect 1)\n", r);

	r = clear_accessed_dirty(buf, 3 * 4096, PT_CLEAR_D);
	printf("[TEST] valid clear D -> %d (expect 0)\n", r);
    r = check_accessed_dirty(buf, 3 * 4096, PT_CLEAR_D);
	printf("[TEST] Check D -> %d (expect 0)\n", r);

    printf("\n\nPAGETABLE AFTER CLEAR ------------------\n");
    dump_pagetable();

    r = check_accessed_dirty(buf, 3 * 4096, PT_CLEAR_A | PT_CLEAR_D);
	printf("[TEST] Check A | D after clear -> %d (expect 0)\n", r);

	free(buf);

    printf("\n\nPAGETABLE AFTER FREE -------------------\n");
    dump_pagetable();
	exit(0);
}