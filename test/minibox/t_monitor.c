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

struct thread_info {
	pthread_t	 thread_id;
	int	         thread_num;
	char	    *argv_str;
};

void *
process(void *arg)
{
	pid_t cpid, w;
	int status;
	struct thread_info *tinfo = arg;

	printf("Trying to execute thread %s\n", tinfo->argv_str);

/*
	cpid = fork();
	if(cpid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if(cpid == 0) {
		printf("Child PID is %ld\n", (long) getpid());

		execl(tinfo->argv_str, tinfo->argv_str, NULL);

	} else {
		do {
			w = waitpid(cpid, &status, WUNTRACED | WCONTINUED);	
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
		} while(!WIFEXITED(status) && !WIFSIGNALED(status));
		return status;
	}
*/

	printf("Exiting thread %s\n", tinfo->argv_str);
	return 0;
}

int
main(int argc, char *argv[])
{
	struct thread_info *tinfo;
	pthread_attr_t attr;
	int i, s, num_threads;
	void *res;

	num_threads = argc - 1;
	printf("Monitor started with %d processes\n", num_threads);

	/* Initiallize thread creation attributes */

	s = pthread_attr_init(&attr);
	if(s != 0)
		handle_error_en("pthread_attr_init", s);

	/* Allocate memory for pthread_create() arguments */

	tinfo = calloc(num_threads, sizeof(struct thread_info));
	if(tinfo == NULL)
		handle_error("calloc");

	/* Create one thread for each command-line argument */

	for(i=0; i<num_threads; i++) {
		tinfo[i].thread_num = i + 1;
		tinfo[i].argv_str = argv[i + 1];

		s = pthread_create(&tinfo[i].thread_id, &attr,
		                   &process, &tinfo[i]);
		if(s != 0)
			handle_error_en("pthread_create", s);
	}

	/* Destroy the thread attribes object, since it is no
	   longer needed */
	
/*
	s = pthread_attr_destroy(&attr);
	if(s != 0)
		handle_error_en("pthread_attr_destroy", s);
*/

	/* Join with each thread, and display its returned value */

/*
	for(i = 0; i < num_threads; i++) {
		s = pthread_join(tinfo[i].thread_id, &res);
		if(s != 0)
			handle_error_en("pthread_join", s);

		printf("Joined with thread %d; return value was %s\n",
		        tinfo[i].thread_num, (char *)res);
		free(res);
	}

	free(tinfo);
	exit(EXIT_SUCCESS);
*/
}
