#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "../include/common.h"

typedef struct {
    char name[21];
    unsigned long read;
    unsigned long write;
} diskdev;

static void readlines(int n, char ***linearr, size_t *bufsize, FILE *fpath);
static int rwloop(void);
static int countlines(FILE *fpath);
static int isnum(char ch2chk);
static int isdevice(char *str);
static int countdevices(int n, char **linearr);
static diskdev* getdiskdata(diskdev *disks, char **str, int nlines);


/*
 * ARGS:
 * - arg1: max number of lines to read,
 * - arg2: array to modify
 * - arg4: buffersize to use
 * - arg3: file path to read.
 * RETURNS:
 * - nothing
 */
void
readlines(int n, char ***linearr, size_t *bufsize, FILE *fp)
{
	ssize_t nchars; /* num of read characters per line */
	int lineno;

	for (lineno = 0; lineno < n; lineno++) {
		nchars = getline(&(*linearr)[lineno], bufsize, fp);

		if (nchars < 0)
			break;

		/* getline() reads newline, remove it */
		if ((*linearr)[lineno][nchars-1] == '\n')
			(*linearr)[lineno][nchars-1] = '\0';

	}
	if (lineno == 1 && nchars < 0)
		die("Given file has no lines to read.");
}
int
countlines(FILE *fpath)
{
	int chr, nline = 0;
	while ((chr = fgetc(fpath)) != EOF)
		if (chr == '\n')
			nline++;
	/* seek back to start of the file */
	fseek(fpath, 0, SEEK_SET);
	return nline;
}
int
countdevices(int n, char **linearr)
{
	int ndevice = 0;
	char name[21];
	for (int i = 0; i < n; i++) {
		sscanf(linearr[i], "%*u%*u%20s", name);
		if (isdevice(name))
			ndevice++;
	}
	return ndevice;
}

int
isdevice(char *str)
{
	/* Naming of nvme devices is nvmeAnB.
	 * A is the controller number, B is the number of the drive connected
	 * to the controller A.
	 *
	 * For sata devices, naming convention is sdA.
	 * A is the alphabetical number starting from a ending at z. If z is
	 * exceeded it turns to aa, then ab and so on.
	 */
	int len = strlen(str);
	/* sda, sdb, ..., sdaa */
	if (str[0] == 's' && str[1] == 'd' && !isnum(str[len-1]))
		return 1;
	/* nvme0n1, nvme0n2, ... nvme9n10 */
	else if (str[0] == 'n' && str[1] == 'v' && str[len-2] != 'p')
		return 1;
	else
		return 0;
}

/* This function increases the number of disks as found. */
diskdev*
getdiskdata(diskdev *disks, char **str, int nlines)
{
	char name[21];
	unsigned long rd, wrt;
	int ndsk = 0;

	/* go through devices/partitions get data of devices */
	for (int i = 0; i < nlines; i++) {
		sscanf(str[i], "%*u%*u%20s%*u%*u%lu%*u%*u%*u%lu", name, &rd, &wrt);
		if (isdevice(name)) {
			/* deep copy name string */
			if (memccpy(disks[ndsk].name, name, '\0', 20) == NULL)
				disks[ndsk].name[20] = '\0';
			disks[ndsk].read = rd;
			disks[ndsk].write = wrt;
			ndsk++;
		}
	}
	return disks;
}

int
rwloop(void)
{
	FILE *fp = NULL;
	int nline, ndisk;
	size_t bufsize = 100; /* initial malloc string length */
	double conv2mbs = 5e-4/sleepamt, rspd, wspd;


	/* Initialization of disk statistics */
	fp = fopen("/proc/diskstats", "r");
	if (fp == NULL) {
		fprintf(stderr, "ERROR: cannot open the file.\n");
		exit(1);
	}

	/* count number of lines in the file */
	nline = countlines(fp);
	/* seek back to start of the file */
	fseek(fp, 0, SEEK_SET);

	/* Generate data structure to hold file's contents */
	char *filedata[nline];

	for (int j = 0; j < nline; j++) {
		filedata[j] = (char*)malloc(200*sizeof(char));
	}
	/* copy  nline amount of lines from the file to the memory */
	char **ptr = filedata;
	readlines(nline, &ptr, &bufsize, fp);
	/* count number of disks, sdA, nvmeAnB... */
	ndisk = countdevices(nline, filedata);
	/* Generate data structures to hold disk information */
	diskdev disk[ndisk], olddisk[ndisk];
	/* Read filedata nline times and write to disk data structure */
	getdiskdata(disk, filedata, nline);

	/* register signal handler for SIGINT */
	signal(SIGINT, setflag);
	while (!rwflag) {
		/* seek back to start of the file */
		fseek(fp, 0, SEEK_SET);

		for (int j = 0; j < ndisk; j++) {
			olddisk[j].read  = disk[j].read;
			olddisk[j].write = disk[j].write;
		}
		sleep(sleepamt);

		/* If number of devices in the file changes, quit. This is done
		 * to avoid checking and redefining everything from scratch.
		 * Instead it is better to restart the function we're in;
		 * this way it's easier to read and follow the code.
		 */
		if (countlines(fp) != nline) {
			for (int i = 0; i < nline; i++)
				free(filedata[i]);
			fclose(fp);
			return 1;
		}

		readlines(nline, &ptr, &bufsize, fp);
		getdiskdata(disk, filedata, nline);

		for (int k = 0; k < ndisk; k++) {
			rspd = (disk[k].read - olddisk[k].read)*conv2mbs;
			wspd = (-olddisk[k].write + disk[k].write)*conv2mbs;
			printf("%-8s R/W: %.1f %.1f M/s\n", disk[k].name, rspd, wspd);
		}
		putchar('\n');
	}
	if (rwflag)
		printf("SIGINT received, exiting.\n");
	/* free dynamically allocated memories used by getline call */
	for (int j = 0; j < nline; j++)
		free(filedata[j]);
	fclose(fp);
	return 0;
}

int
isnum(char ch2chk)
{
	if (ch2chk >= 48 && ch2chk <= 57)
		return 1;
	else
		return 0;
}

int
alldiskrw(void)
{
	while(1)
		switch (rwloop()) {
		case 0: /* rwloop sucesfully exits (SIGNAL received) */
			return 0;
		case 1:
			fprintf(stderr, "Device changes detected.\n");
			continue;
		default:
			return 0;
		}
}
