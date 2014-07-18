#include <unistd.h>
#include <stdio.h>
#include <sys/syscall.h>

#include "colors.h"

int main () { 
	int id;

	id = syscall(SYS_gettid);

	fprintf(stderr,
			ANSI_COLOR_BOLD_MAGENTA 
			"Thread ID = %d" 
			ANSI_COLOR_RESET 
			"\n",
			id);
	fflush(0);

	return 0;
}
