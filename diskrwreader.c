#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>



static char* setpath(char *diskname);
static void setflag();
static void readline(char *targetstr, size_t allocsize, FILE *fpath);
static void rwloop(char *path);
static long str2pi(char *flag, char *slptime);
static void usage();


static int  PATHSIZE = 128;
static long SLEEPAMT = 1;
volatile sig_atomic_t rwflag = 0;


/* This function reads the given argument diskname and transforms it
 * into the path ready to be read.
 */
char*
setpath(char *diskname)
{
        int wrtnbyte;
	char *path = (char*)malloc(PATHSIZE*sizeof(char));

	if (path == NULL) {
		fprintf(stderr, "ERROR: cannot alocate memory.\n");
		exit(1);
	}
	/* set the path for the given disk */
	wrtnbyte = snprintf(path, PATHSIZE, "/sys/block/%s/stat", diskname);

	if (wrtnbyte < 0 || wrtnbyte >= PATHSIZE) {
		fprintf(stderr, "Error: too long path size.\n");
		exit(1);
	}
	return path;
}
void
setflag()
{
	rwflag = 1;
}

void
readline(char *targetstr, size_t tstrlen, FILE *fpath)
{
	ssize_t nchars;
	nchars = getline(&targetstr, &tstrlen, fpath);

	if (nchars < 0) {
		fprintf(stderr, "ERROR: failure to read given disk data\n");
		free(targetstr);
		exit(1);
	}
}

void
rwloop(char *path)
{
	FILE *fp = NULL;
	/* sizeof(char) is 1, no need to specify, similar to getline() impl. */
	size_t ndiskstat = 200;
	char *diskstat = (char *) malloc(ndiskstat*sizeof(char));;
	unsigned long rwsector[2][2] = {{0, 0}, {0, 0}};
	float rwspeed[2] = {0, 0};
	float conv2mbs = 5e-4/SLEEPAMT;

	/* Initialization of disk statistics */
	fp = fopen(path, "r");
	if (fp == NULL) {
		fprintf(stderr, "ERROR: failure to open requested file.\n");
		exit(1);
	}

	/* Set disk values to memory */
	readline(diskstat, ndiskstat, fp);
	fclose(fp);

	/* Set values for reading/writing counts in terms of sectors */
	sscanf(diskstat, "%*u %*u %lu %*u %*u %*u %lu", &rwsector[0][1], &rwsector[1][1]);


	signal(SIGINT, setflag);
	/* register signal handler for SIGINT */
	while (!rwflag) {
		sleep(SLEEPAMT);
		for (int i = 0; i < 2; i++)
			rwsector[i][0] = rwsector[i][1];
		fp = fopen(path, "r");
		readline(diskstat, ndiskstat, fp);
		fclose(fp);

		sscanf(diskstat, "%*u %*u %lu %*u %*u %*u %lu", &rwsector[0][1], &rwsector[1][1]);

		for (int i = 0; i < 2; i++)
			rwspeed[i] = (rwsector[i][1]-rwsector[i][0])*conv2mbs;

		printf("READ: %.1f MB/s, WRITE: %.1f MB/s\n", rwspeed[0], rwspeed[1]);
	}
	if (rwflag)
		printf("SIGINT received, exiting.\n");
	free(diskstat);
}

/* Converts strings to postive integers */
long
str2pi(char *flag, char *slptime)
{

	char *ptr = NULL;
	long retval = 1;
	retval = strtol(slptime, &ptr, 10);

	if (ptr == slptime) {
		fprintf(stderr, "ERROR: Not a decimal in %s.\n", flag);
		usage();
	} else if (*ptr != '\0') {
		fprintf(stderr, "ERROR: Extra values in %s.\n", flag);
		usage();
	} else if (retval < 1 || retval > INT_MAX) { /* INT_MAX's enough */
		fprintf(stderr, "ERROR: Bad value for %s.\n", flag);
		usage();
	}
	return retval;
}

void
usage()
{
	fprintf(stderr, "Usage: diskmon <diskname> [-t <int>]\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	if (argc < 2)
		usage();

	for (int i = 2; i < argc; i++)
		/* if -t is given, check there is a following value exists */
		if (!strcmp(argv[i], "-t") && argv[++i] && *argv[i] != '\0')
			SLEEPAMT = str2pi(argv[i-1], argv[i]);

	char *path = setpath(argv[1]);
	rwloop(path);
	return 0;
}
