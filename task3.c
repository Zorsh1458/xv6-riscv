#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int
main(int argc, char *argv[])
{
	int pipefd[2];
	pid_t pid;

	if (pipe(pipefd) < 0) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	pid = fork();

	if (pid < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid == 0) {
		close(pipefd[1]);

		char buf[1024];
		ssize_t n;
		while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
			buf[n] = '\0';
			printf("%s", buf);
		}

		if (n < 0) {
			perror("read");
			close(pipefd[0]);
			exit(EXIT_FAILURE);
		}

		close(pipefd[0]);
		exit(EXIT_SUCCESS);
	}

	close(pipefd[0]);

	for (int i = 1; i < argc; i++) {
		size_t len = strlen(argv[i]);
		size_t done = 0;
		while (1) {
			int n = write(pipefd[1], argv[i] + done, len);
			if (n < 1) {
				perror("write");
				close(pipefd[1]);
				exit(EXIT_FAILURE);
			}
			done += n;
			len -= n;
			if (len == 0) {
				break;
			}
		}

		if (write(pipefd[1], "\n", 1) < 1) {
			perror("write");
			close(pipefd[1]);
			exit(EXIT_FAILURE);
		}
	}

	close(pipefd[1]);

	int status;
	if (waitpid(pid, &status, 0) < 0) {
		perror("waitpid");
		exit(EXIT_FAILURE);
	}

	if (WIFEXITED(status)) {
		printf("Child (pid %d) exited with status %d\n", pid, WEXITSTATUS(status));
	} else if (WIFSIGNALED(status)) {
		printf("Child (pid %d) killed by signal %d\n", pid, WTERMSIG(status));
	}

	exit(EXIT_SUCCESS);
}