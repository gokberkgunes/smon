#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PATHSIZE 128
#define SLEEPAMOUNT 1


char* setpath(char *diskname);
static void rwloop(char *path);
static void readline(char **targetstr, FILE *fpath);

int
main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <diskname>\n", argv[0]);
		return 1;
	}

	char *path = setpath(argv[1]);
	rwloop(path);

	return 0;
}



void
rwloop(char *path)
{
	FILE *fp;
	char *diskstat = NULL;
	unsigned long rwsectors[2][2] = {{0, 0}, {0, 0}};
	float rwspeed[2];
	float multiplier = 5e-4/SLEEPAMOUNT;



	fp = fopen(path, "r");
	if (fp == NULL) {
		fprintf(stderr, "Error: failure to open requested file.\n");
		exit(1);
	}
	readline(&diskstat, fp);
	fclose(fp);

	/* Set initial values */
	sscanf(diskstat, "%*u %*u %lu %*u %*u %*u %lu", &rwsectors[0][1], &rwsectors[1][1]);


	while (1) {
		sleep(SLEEPAMOUNT);

		//printf("1: %lu MB/s 1: %lu MB/s\n", rwsectors[0][1], rwsectors[1][1]);

		for (int i = 0; i < 2; i++)
			rwsectors[i][0] = rwsectors[i][1];

		//printf("0: %lu MB/s 0: %lu MB/s\n", rwsectors[0][0], rwsectors[1][0]);

		fp = fopen(path, "r");
		readline(&diskstat, fp);
		fclose(fp);

		sscanf(diskstat, "%*u %*u %lu %*u %*u %*u %lu", &rwsectors[0][1], &rwsectors[1][1]);

		for (int i = 0; i < 2; i++)
			rwspeed[i] = (rwsectors[i][1]-rwsectors[i][0])*multiplier;

		printf("Read: %.1f MB/s Write: %.1f MB/s\n", rwspeed[0], rwspeed[1]);
	}

	free(diskstat);
}

char*
setpath(char *diskname)
{
        /* This function reads the given argument diskname and transforms it
         * into the path ready to be read */
        size_t wrtnbyte;
	char *path;

	path = (char*)malloc(PATHSIZE*sizeof(char));

	if (path == NULL) {
		fprintf(stderr, "Error: cannot alocate memory.\n");
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
readline(char **targetstr, FILE *fpath)
{
	ssize_t nchars;
	size_t allocsize = 256*sizeof(char);

	nchars = getline(targetstr, &allocsize, fpath);
	if (nchars < 0) {
		fprintf(stderr, "Error: failure to read given disk data\n");
		/* free the memory if not NULL */
		if (targetstr != NULL)
			free(targetstr);
		exit(1);
	}
}

