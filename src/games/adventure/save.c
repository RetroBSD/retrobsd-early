/*
 * save (III)   J. Gillogly
 * save user core image for restarting
 * usage: save(<command file (argv[0] from main)>,<output file>)
 * bugs
 *   -  impure code (i.e. changes in instructions) is not handled
 *      (but people that do that get what they deserve)
 */
#include <unistd.h>
#include <fcntl.h>
#include "hdr.h"

#define MAGIC 0x76646121

struct file_header {
        unsigned magic;
        unsigned msg_size;
        unsigned data_size;
        unsigned heap_size;
        char *data_start;
        char *heap_start;
};

long filesize = sizeof(struct file_header); /* accessible to caller     */

extern char __data_start[], _end[], *heap_start;

int
save(outfile)                           /* save data image              */
char *outfile;
{
	register int fd;
        char *heap_end;
        struct file_header hdr;

	fd = open(outfile, O_RDWR);
	if (fd < 0) {
	        printf("Cannot open %s\n", outfile);
		return(-1);
	}
	if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
	        printf("Save file empty\n");
		return(-1);
	}
	if (hdr.msg_size == 0) {
                hdr.msg_size = (lseek(fd, 0, SEEK_END) + 7) & ~7;
                printf("Message size = %u\n", hdr.msg_size);
        }
	hdr.data_start = __data_start;
        hdr.data_size  = _end - hdr.data_start;

	heap_end = (char*) sbrk(0);
	hdr.heap_start = heap_start;
	hdr.heap_size  = heap_end - heap_start;

	hdr.magic = MAGIC;
        lseek(fd, 0, SEEK_SET);
	write(fd, &hdr, sizeof(hdr));

        lseek(fd, hdr.msg_size, SEEK_SET);
	write(fd, hdr.data_start, hdr.data_size);
	write(fd, heap_start, hdr.heap_size);

        printf("Saved %u bytes to %s\n", hdr.msg_size +
                hdr.data_size + hdr.heap_size, outfile);
	close(fd);
        return 0;
}

void
restore(fd)
{
        struct file_header hdr;

	if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr) ||
            hdr.magic != MAGIC ||
	    hdr.data_start != __data_start ||
            hdr.data_size  != _end - __data_start ||
            hdr.heap_start != heap_start) {
	        printf("Saved data invalid\n");
		exit(-1);
	}
	brk(hdr.heap_start + hdr.heap_size);
        lseek(fd, hdr.msg_size, SEEK_SET);
	read(fd, __data_start, hdr.data_size);
	read(fd, heap_start, hdr.heap_size);
}
