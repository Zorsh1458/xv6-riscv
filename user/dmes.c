#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
	if (argc < 2) {
		printf("dmes <list|set>\n");
		exit(0);
	}

	if (strcmp(argv[1], "list") == 0) {
    	char messages[2048];

		int n = dmesg(messages, sizeof(messages));
		if (n < 0) {
			printf("dmesg: error %d\n", n);
			exit(1);
		}

		printf("Message log:\n");
    	write(1, messages, n);
	} else if (strcmp(argv[1], "set") == 0) {
		if (argc < 4) {
			printf("dmes set <syscalls|interruptions|forks|executions> <duration: 0 to disable, -1 to forever>\n");
			exit(0);
		}

		int flag = -1;
		if (strcmp(argv[3], "-1")) {
			flag = atoi(argv[3]);
		}

		if (strcmp(argv[2], "syscalls") == 0) {
			set_diag(0, flag);
		}
		if (strcmp(argv[2], "interruptions") == 0) {
			set_diag(1, flag);
		}
		if (strcmp(argv[2], "forks") == 0) {
			set_diag(2, flag);
		}
		if (strcmp(argv[2], "executions") == 0) {
			set_diag(3, flag);
		}
		
		if (flag == -1) {
			printf("Enabling logging of %s\n", argv[2]);
		} else if (flag == 0) {
			printf("Disabling logging of %s\n", argv[2]);
		} else {
			printf("Enabling logging of %s for %d ticks\n", argv[2], flag);
		}
	}

    exit(0);
}