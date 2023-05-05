#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>



static char* setpath(char *diskname);
static void setflag(int signum);
static void readline(char **targetstr, FILE *fpath);
static void rwloop(char *path);
static void sslptime(char *flag, char *slptime);
static void usage();


static int  PATHSIZE = 128;
static long SLEEPAMT = 1;
volatile sig_atomic_t rwflag = 0;


char*
setpath(char *diskname)
{
        /* This function reads the given argument diskname and transforms it
         * into the path ready to be read */
        size_t wrtnbyte;
	char *path;

	path = (char*)malloc(PATHSIZE*sizeof(char));

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
setflag(int signum)
{
	rwflag = 1;
}

void
readline(char **targetstr, FILE *fpath)
{
	ssize_t nchars;
	size_t allocsize = 256*sizeof(char);

	nchars = getline(targetstr, &allocsize, fpath);
	if (nchars < 0) {
		fprintf(stderr, "ERROR: failure to read given disk data\n");
		/* free the memory if not NULL */
		if (targetstr != NULL)
			free(targetstr);
		exit(1);
	}
}

void
rwloop(char *path)
{
	FILE *fp;
	char *diskstat = NULL;
	unsigned long rwsector[2][2] = {{0, 0}, {0, 0}};
	float rwspeed[2] = {0, 0};
	float conv2mbs = 5e-4/SLEEPAMT;



	/* Open the path to allow initialization of disk statistics */
	fp = fopen(path, "r");
	if (fp == NULL) {
		fprintf(stderr, "ERROR: failure to open requested file.\n");
		exit(1);
	}

	/* Set disk values to memory */
	readline(&diskstat, fp);
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
		readline(&diskstat, fp);
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

/* Gets -t flag's value and set it to global variable SLEEPAMT */
void
sslptime(char *flag, char *slptime)
{

	char *ptr;
	SLEEPAMT = strtol(slptime, &ptr, 10);

	if (*ptr != '\0') {
		fprintf(stderr, "ERROR: Non-integer value for <%s>.\n", flag);
		usage();
	} else if (SLEEPAMT < 0 || SLEEPAMT > INT_MAX) {
		fprintf(stderr, "ERROR: Bad value for <%s>.\n", flag);
		usage();
	}
}

void
usage()
{
	fprintf(stderr, "\nUsage: diskmon <diskname> [-t <int>]\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	if (argc < 2)
		usage();

	for (int i = 2; i < argc; i++)
		if (!strcmp(argv[i], "-t") && argv[++i] && *argv[i] != '\0')
			sslptime(argv[i-1], argv[i]);

	char *path = setpath(argv[1]);
	rwloop(path);
	return 0;
}
