#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <signal.h>
#include "../include/common.h"

static char* setdiskpath(char *diskname);
static void readline(char **targetstr, size_t *tstrlen, FILE *fpath);
static void rwloop(char *path);
static int PATHSIZE = 128;


/* This function reads the given argument diskname and transforms it
 * into the path ready to be read.
 */
char*
setdiskpath(char *diskname)
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
readline(char **targetstr, size_t *tstrlen, FILE *fpath)
{
	ssize_t nchars;
	nchars = getline(targetstr, tstrlen, fpath);

	if (nchars < 0) {
		free(*targetstr);
		die("ERROR: failure to read given disk data file.");
	}
}

void
rwloop(char *path)
{
	FILE *fp = NULL;
	/* 300 characters should be more than enough to store line */
	size_t ndiskstat = 300*sizeof(char);
	char *diskstat = (char *) malloc(ndiskstat*sizeof(char));;
	unsigned long rwsector[2][2] = {{0, 0}, {0, 0}};
	float rwspeed[2] = {0, 0};


	/* Initialization of disk statistics */
	fp = fopen(path, "r");
	if (fp == NULL)
		die("ERROR: failure to open requested file, %s.", path);

	/* Set disk values to memory */
	readline(&diskstat, &ndiskstat, fp);
	fclose(fp);

	/* Set values for reading/writing counts in terms of sectors */
	sscanf(diskstat, "%*u %*u %lu %*u %*u %*u %lu", &rwsector[0][1], &rwsector[1][1]);


	signal(SIGINT, setflag);
	/* register signal handler for SIGINT */
	while (!rwflag) {
		sleep(sleepamt);
		for (int i = 0; i < 2; i++)
			rwsector[i][0] = rwsector[i][1];
		fp = fopen(path, "r");
		readline(&diskstat, &ndiskstat, fp);
		fclose(fp);

		sscanf(diskstat, "%*u %*u %lu %*u %*u %*u %lu", &rwsector[0][1], &rwsector[1][1]);

		for (int i = 0; i < 2; i++)
			rwspeed[i] = (rwsector[i][1]-rwsector[i][0])*sec2mbps;

		printf("READ: %.1f MB/s, WRITE: %.1f MB/s\n", rwspeed[0], rwspeed[1]);
	}
	if (rwflag)
		printf("SIGINT received, exiting.\n");
	free(diskstat);
}



int
main(int argc, char *argv[])
{
	char *path  = NULL;

	for (int i = 1; i < argc; i++) {
		/* if -t is given, check there is a following value exists */
		if (!strcmp(argv[i], "-t") && argv[i+1] && *argv[i+1] != '\0') {
			i++;
			sleepamt = arg2pi(argv[i-1], argv[i]);
			sec2mbps = 5e-4/sleepamt;
		} else if (!strcmp(argv[i], "-d") && argv[i+1] && *argv[i+1] != '\0') {
			i++;
			path = setdiskpath(argv[i]);
		}
		else {
			die("%s", "ERROR: Bad argument given.");
		}
	}
	if (path != NULL) {
		rwloop(path);
		free(path);
	} else {
		alldiskrw();
	}
	return 0;
}
