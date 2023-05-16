#include <stdarg.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "../include/common.h"

volatile sig_atomic_t rwflag = 0;
long sleepamt = 1;
float sec2mbps = 5e-4; /* sector to mbps conversion variable */

void
setflag(int signum)
{
	(void)signum; /* unused */
	rwflag = 1;
}

void
die(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputc('\n', stderr);

	//fprintf(stderr, "Usage: diskmon <diskname> [-t <int>]\n");
	exit(1);
}

/* Converts arguments (strings) to postive integers safely */
long
arg2pi(char *flag, char *val)
{
	char *ptr = NULL;
	long retval = 1;
	retval = strtol(val, &ptr, 10);
	if (ptr == val)
		die("ERROR: Not a decimal in %s.", flag);
	else if (*ptr != '\0')
		die("ERROR: Extra values in %s.", flag);
	else if (retval < 1 || retval > INT_MAX) /* INT_MAX's enough */
		die("ERROR: Bad value for %s.", flag);
	return retval;
}

