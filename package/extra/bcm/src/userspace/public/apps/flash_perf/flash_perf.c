/*
<:copyright-BRCM:2019:DUAL/GPL:standard

   Copyright (c) 2019 Broadcom
   All Rights Reserved

Unless you and Broadcom execute a separate written software license
agreement governing use of this software, this software is licensed
to you under the terms of the GNU General Public License version 2
(the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
with the following added to such license:

   As a special exception, the copyright holders of this software give
   you permission to link this software with independent modules, and
   to copy and distribute the resulting executable under terms of your
   choice, provided that you also meet, for each linked independent
   module, the terms and conditions of the license of that module.
   An independent module is a module which is not derived from this
   software.  The special exception does not apply to any modifications
   of the software.

Not withstanding the above, under no circumstances may you combine
this software in any way with any other Broadcom software provided
under a license other than the GPL, without Broadcom's express prior
written consent.

:>
*/
#define _FILE_OFFSET_BITS 64

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <unistd.h>
#include <getopt.h>             /* getopt_long() */
#include <sys/stat.h>
#include <linux/fs.h>
#include <ctype.h>

#define __USE_GNU
#include <fcntl.h>


#define WRITE_BUF_SIZE (8 * 1024 * 1024)
#define READ_BUF_SIZE (8 * 1024 * 1024)

int write_buf_size = WRITE_BUF_SIZE;
int read_buf_size = READ_BUF_SIZE;

char *myname;
int cline_no_rdirect;
int cline_no_wdirect;
int cline_readonly;
int cline_writeonly;
int cline_tsb;
int cline_altpat;
int cline_incpat;
int cline_zeropat;
int cline_loop;
char *testfile_name;
off_t testfile_size = 1024 * 1024 * 128;

/*
 * Read a number with a possible multiplier.
 * Returns -1 if the number format is illegal.
 */
static int getnum(char *cp, off_t *value)
{
	char *str = cp;

	if (!isdigit(*str))
		goto bad;
	*value = 0;
	while (isdigit(*str))
		*value = *value * 10 + *str++ - '0';

	switch (tolower(*str++)) {
	case 'g':
		*value *= (1024 * 1024 * 1024);
		break;
	case 'm':
		*value *= (1024 * 1024);
		break;
	case 'k':
		*value *= 1024;
		break;
	case 'b':
		*value *= 512;
		break;
	case 'w':
		*value *= 2;
		break;
	case '\0':
		return 0;
	default:
		goto bad;
	}

	if (*str)
		goto bad;
	return 0;

bad:
	printf("Bad size: %s\n", cp);
	return -1;
}

double delta_time(struct timeval *start, struct timeval *end)
{
	double startval;
	double endval;

	startval = (double)start->tv_sec + ((double)start->tv_usec / 1000000);
	endval = (double)end->tv_sec + ((double)end->tv_usec / 1000000);
	return (endval - startval);
}

void change_pat_buffer(char *buffer, int size, int urandom_fd)
{
	unsigned long *ptr = (unsigned long *)buffer;
	int x;
	unsigned long modifier;

	read(urandom_fd, &modifier, sizeof(modifier));
	for (x = size / sizeof(unsigned long); x-- > 0; ptr++)
		*ptr += modifier;
}

void do_write_test(char *filename, off_t total_size)
{
	int test_fd;
	int ran_fd;
	char *pat_buf = NULL;
	int write_size;
	int status;
	off_t size;
	struct timeval start_time;
	struct timeval end_time;
	double delta;
	int mode;
	int pass = 0;
	long long total_bytes = 0;
	int retries = 0;

	/* Clear out the disk cache so we actually read the card */
	test_fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
	if (test_fd == -1) {
		printf("Error clearing system disk cache, continuing anyway\n");
	} else {
		sleep(1);
		status = write(test_fd, "1\n", 2);
		if (status != 2)
			printf("Error doing drop caches\n");
	}
	printf("Creating test file: %s, size: %lld\n", filename, (long long)total_size);
	unlink(filename);
	if (!cline_no_wdirect)
		mode = O_RDWR | O_CREAT | O_TRUNC | O_DIRECT;
	else
		mode = O_RDWR | O_CREAT | O_TRUNC;
	test_fd = open(filename, mode, 0777);
	if (test_fd == -1) {
		perror("Error creating test file");
		exit(1);
	}
	ran_fd = open("/dev/urandom", O_RDONLY);
	if (ran_fd == -1) {
		perror("Error opening random device");
		exit(1);
	}

	/* Direct IO requires that buffers are aligned to the driver's min block size */
	status = posix_memalign((void **)&pat_buf, 4096, write_buf_size);
	if (pat_buf == NULL) {
		perror("");
		exit(1);
	}
	if (cline_altpat) {
		unsigned long *lptr = (unsigned long *)pat_buf;
		int x = write_buf_size / sizeof(long);

		while (x-- > 0)
			*lptr++ = 0xf0f0f0f0;
	} else if (cline_incpat) {
		unsigned long *lptr = (unsigned long *)pat_buf;
		int x = write_buf_size / sizeof(long);
		int pat = 0;

		while (x-- > 0)
			*lptr++ = pat++;
	} else if (cline_zeropat) {
		unsigned long *lptr = (unsigned long *)pat_buf;
		int x = write_buf_size / sizeof(long);

		while (x-- > 0)
			*lptr++ = 0;
	} else {
		status = read(ran_fd, pat_buf, write_buf_size);
		if (status != write_buf_size) {
			printf("Error reading random device, status: %d\n",
			       status);
			perror("");
			exit(1);
		}
	}
	do {
		gettimeofday(&start_time, NULL);
		size = total_size;
		lseek(test_fd, 0, SEEK_SET);
		while (size) {
			if (size > write_buf_size)
				write_size = write_buf_size;
			else
				write_size = size;
			status = write(test_fd, pat_buf, write_size);
			if (status < 0) {
				perror("Error writing test file");
				exit(1);
			}
			if (status != write_size) {
				printf("Error writing test file\n");
				printf("write returned %d, expected %d\n",
					status, write_size);
			}
			if (status == 0) {
				if (retries++ == 10) {
					printf("Error: write retrys expired\n");
					exit(1);
				}
			}
			size -= status;
		}
		fsync(test_fd);
		gettimeofday(&end_time, NULL);
		delta = delta_time(&start_time, &end_time);
		if (cline_tsb) {
			total_bytes += total_size;
			printf("Pass %d, total GigaBytes: %.3f  GiBs: %.3f\n", ++pass,
				   (double)total_bytes / (1000 * 1000 * 1000),
			       (double)total_bytes / (1024 * 1024 * 1024));
			change_pat_buffer(pat_buf, write_buf_size, ran_fd);
		}
		printf("Wrote %jd bytes in %.3f seconds, rate: %.3f MBytes/s %.3f MiB/s\n",
			   total_size, delta,
			   (double)total_size / delta / (1000 * 1000),
			   (double)total_size / delta / (1024 * 1024));
	} while (cline_tsb);
	printf("Test file %s created, size %jd\n", filename, total_size);
	free(pat_buf);
	close(test_fd);
	close(ran_fd);
}


void do_read_test(char *file)
{
	int fd;
	int status;
	int mode;
	struct timeval start_time;
	struct timeval end_time;
	double delta;
	off_t size;
	off_t total_size;
	int read_size;
	struct stat statbuf;
	char *pat_buf = NULL;

	/* Clear out the disk cache so we actually read the card */
	fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
	if (fd == -1) {
		printf("Error clearing system disk cache, continuing anyway\n");
	} else {
		sleep(1);
		status = write(fd, "1\n", 2);
		if (status != 2)
			printf("Error doing drop caches\n");
	}
	if (!cline_no_rdirect)
		mode = O_RDONLY | O_DIRECT;
	else
		mode = O_RDONLY;
	if (stat(file, &statbuf) != 0) {
		fprintf(stderr, "Test file: %s, does not exist\n",
			file);
		exit(1);
	}
	size = total_size = statbuf.st_size;

	/* Special devices return 0, so set to 1G to allow reads */
	if (size == 0)
		size = total_size = 1 * 1024 * 1024 * 1024;
	fd = open(file,  mode);
	if (fd == -1) {
		fprintf(stderr, " %s: could not open %s, ", myname, file);
		perror("");
		exit(1);
	}

	/* Direct IO requires that buffers are aligned to the driver's min block size */
	status = posix_memalign((void **)&pat_buf, 4096, write_buf_size);
	if (pat_buf == NULL) {
		perror("");
		exit(1);
	}
	gettimeofday(&start_time, NULL);
	printf("Starting read performance test\n");
	while (size) {
		if (size > read_buf_size)
			read_size = read_buf_size;
		else
			read_size = size;
		status = read(fd, pat_buf, read_size);
		if (status != read_size) {
			perror("Error reading test file");
			exit(1);
		}
		size -= read_size;
	}
	gettimeofday(&end_time, NULL);
	delta = delta_time(&start_time, &end_time);
	printf("Copied %jd bytes in %.3f seconds, rate: %.3f MBytes/s %.3f MiB/s\n",
		   total_size, delta,
		   (double)total_size / delta / (1000 * 1000),
		   (double)total_size / delta / (1024 * 1024));
	free(pat_buf);
	close(fd);
}



static void usage(FILE *fp, const char *name)
{
	fprintf(fp,
	    "Usage: %s [options] <path_to_test_file>\n\n"
	    "Options:\n"
	    "-a, --altpat                Use alternating 0,1 for write data\n"
	    "-b, --blocksize=<size>      Size of read/write buffer. When\n"
	    "                            using DirectIO this is usually the\n"
	    "                            size of the read/write to the device\n"
	    "-h, --help                  Print this message\n"
	    "-i, --incpat                Use incrementing pattern write data\n"
	    "-l, --loop                  Run continuously\n"
	    "-n, --no-read-direct        Don't use DirectIO for reads\n"
	    "-r, --readonly              Only do read test, don't write\n"
	    "-s, --size=<size>           Size of test file\n"
	    "-t, --tsb                   Emulate Time Shift Buffer usage\n"
	    "-w, --writeonly             Only do write test, don't read\n"
	    "-x, --no-write-direct       Don't use DirectIO for writes\n"
	    "-z, --zeropat               Use all zeros for write data\n"
	    "\nsizes can have a suffix multiplier of g, m, k, b or w:\n"
	    "\tg - 1024 * 1024 * 1024\n"
	    "\tm - 1024 * 1024\n"
	    "\tk - 1024\n"
	    "\tb - 512\n"
	    "\tw - 2\n",
	    name);
}


static const char short_options[] = "aizhnxrws:tb:l";

static const struct option
long_options[] = {
	{ "altpat",	no_argument,            NULL,           'a' },
	{ "incpat",	no_argument,            NULL,           'i' },
	{ "zeropat",	no_argument,            NULL,           'z' },
	{ "help",	no_argument,            NULL,           'h' },
	{ "no-read-direct", no_argument,        NULL,           'n' },
	{ "no-write-direct", no_argument,       NULL,           'x' },
	{ "readonly",	no_argument,            NULL,           'r' },
	{ "writeonly",	no_argument,            NULL,           'w' },
	{ "size",	required_argument,      NULL,           's' },
	{ "tsb",	no_argument,            NULL,           't' },
	{ "blocksize",	required_argument,      NULL,           'b' },
	{ "loop",	no_argument,            NULL,           'l' },
	{ 0, 0, 0, 0 }
};

int main(int argc, char **argv)
{
	int index;

	myname = argv[0];

	for (;;) {
		int c;

		c = getopt_long(argc, argv,
				short_options, long_options,
				&index);

		if (-1 == c)
			break;

		switch (c) {
		case 0: /* getopt_long() flag */
			break;

		case 'h':
			usage(stdout, argv[0]);
			exit(EXIT_SUCCESS);

		case 'n':
			cline_no_rdirect = 1;
			break;

		case 'x':
			cline_no_wdirect = 1;
			break;

		case 'r':
			cline_readonly = 1;
			break;

		case 'w':
			cline_writeonly = 1;
			break;

		case 's':
			if (getnum(optarg, &testfile_size)) {
				usage(stderr, argv[0]);
				exit(EXIT_FAILURE);
			}
			break;
		case 't':
			cline_tsb = 1;
			break;
		case 'a':
			cline_altpat = 1;
			break;
		case 'i':
			cline_incpat = 1;
			break;
		case 'z':
			cline_zeropat = 1;
			break;
		case 'b':
			{ off_t size;
			if (getnum(optarg, &size)) {
				usage(stderr, argv[0]);
				exit(EXIT_FAILURE);
			}
			write_buf_size = read_buf_size = (int)size;
			}
			break;
		case 'l':
			cline_loop = 1;
			break;
		default:
			usage(stderr, argv[0]);
			exit(EXIT_FAILURE);
		}
	}
	if (argv[optind])
		testfile_name = argv[optind];
	else
		testfile_name = "/mnt/hd/media_test.bin";

	do {
		if (!cline_readonly)
			do_write_test(testfile_name, testfile_size);

		if (!cline_writeonly)
			do_read_test(testfile_name);
	} while (cline_loop);

	return 0;
}
