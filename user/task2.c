#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int pipefd[2];
    int pid;

    if (pipe(pipefd) < 0) {
        printf("pipe failed\n");
        exit(1);
    }

    pid = fork();

    if (pid < 0) {
        printf("fork failed\n");
        exit(1);
    }

    if (pid == 0) {
        close(pipefd[1]);
        
        close(0);
        dup(pipefd[0]);
        
        close(pipefd[0]);
        
        char *wc_argv[] = { "wc", 0 };
        exec("wc", wc_argv);
        
        printf("exec wc failed\n");
        exit(1);
    }

    close(pipefd[0]);

    for (int i = 1; i < argc; i++) {
        write(pipefd[1], argv[i], strlen(argv[i]));
        write(pipefd[1], "\n", 1);
    }

    close(pipefd[1]);

    int status;
    int terminated_pid = wait(&status);

    printf("wc (pid %d) exited with status %d\n", terminated_pid, status);

    exit(0);
}