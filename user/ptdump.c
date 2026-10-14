#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
	if (dump_pagetable() < 0) {
		printf("dump_pagetable failed\n");
		exit(1);
	}
	exit(0);
}