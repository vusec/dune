#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "libdune/dune.h"

#define handle_error_en(msg, en) \
	do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

static void
print_usage() {
	fprintf(stderr, "Usage ./monitor process1 process2 ...\n");
}

int
main(int argc, char *argv[])
{
	pid_t *cpid, w;
	int ret, status, i, n, argnum;

	argnum = argc - 1;

	cpid = malloc(sizeof(pid_t) * argnum);

	for (i = 0; i < argnum; i++) {
		cpid[i] = fork();
		if(cpid[i] == -1) {
			perror("fork");
			exit(EXIT_FAILURE);
		} else if(cpid[i] == 0) {
			//ret = dune_init_and_enter();
			//if (ret) {
			//	perror("FAILED to initialize DUune\n");
			//	exit(ret);
			//}
			//printf("Hello World\n");
			exit(0);
			//execl(argv[i+1], argv[i+1], NULL);
		}
	}

	n = argnum;
	do {
		w = wait(&status);	
		if(w == -1) {
			perror("waitpid");
			exit(EXIT_FAILURE);
		}

		if (WIFEXITED(status)) {
			printf("exited, status=%d\n", WEXITSTATUS(status));
		} else if (WIFSIGNALED(status)) {
			printf("killed by signal %d\n", WTERMSIG(status));
		} else if (WIFSTOPPED(status)) {
			printf("stopped by signal %d\n", WSTOPSIG(status));
		} else if (WIFCONTINUED(status)) {
			printf("continued\n");
		}
		--n;
	} while (n > 0);

	return status;
}
