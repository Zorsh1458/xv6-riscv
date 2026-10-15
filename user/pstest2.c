#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static char *
state_name(int s)
{
	switch(s){
	case PROCINFO_UNUSED:   return "UNUSED  ";
	case PROCINFO_USED:     return "USED    ";
	case PROCINFO_SLEEPING: return "SLEEPING";
	case PROCINFO_RUNNABLE: return "RUNNABLE";
	case PROCINFO_RUNNING:  return "RUNNING ";
	case PROCINFO_ZOMBIE:   return "ZOMBIE  ";
	}
	return "?";
}

int
main(int argc, char *argv[])
{
	int n = ps_listinfo(0, 0);
	if (n < 0) {
		printf("ps_listinfo(NULL) failed: %d\n", n);
		exit(1);
	}

	if (n == 0) {
		printf("no processes\n");
		exit(0);
	}

    // Этот тестовый случай - невалидный адрес записи
	struct procinfo *plist = (struct procinfo *)1;

	int got = ps_listinfo(plist, n);
	if (got < 0 || got >= n) {
		printf("ps_listinfo failed: %d | expected count: %d\n", got, n);
		exit(1);
	}

	printf("pid\tppid\tstate\t\tname\tpname\n");
	for(int i = 0; i < got; i++){
		printf("%d\t%d\t%s\t%s\t%s\n",
		       plist[i].pid,
		       plist[i].ppid,
		       state_name(plist[i].state),
		       plist[i].name,
		       plist[i].pname);
	}

	free(plist);
	exit(0);
}