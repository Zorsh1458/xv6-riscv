#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
    int pid;

    pid = fork();

    if (pid < 0) {
        printf("fork failed\n");
        exit(1);
    }

    if (pid == 0) {
        pause(100);
        exit(1);
    }

    printf("Parent PID: %d, Child PID: %d\n", getpid(), pid);

    int status;
    int terminated_pid = wait(&status);

    printf("Terminated process PID: %d, exit status: %d\n", terminated_pid, status);

    exit(0);
}