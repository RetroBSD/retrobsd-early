/*
 * Disk testing utility.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MEM_SIZE (2*1024*1024)

char *progname;
int verbose;
int fd;

void byte_write (unsigned addr, unsigned char val)
{
    int ret;

    if (verbose)
        printf ("Write byte %02X to address %06X.\n", val, addr);

    ret = lseek (fd, addr, 0);
    if (ret != addr) {
        printf ("Seek error at %06X: result %06X, expected %06X.\n",
            addr, ret, addr);
        exit (-1);
    }
    ret = write (fd, &val, 1);
    if (ret != 1) {
        printf ("Write error at %06X: %s\n", addr, strerror(ret));
        exit (-1);
    }
}

void byte_check (unsigned addr, unsigned char val)
{
    unsigned char rval;
    int ret;

    if (verbose)
        printf ("Read byte from address %06X.\n", addr);

    ret = lseek (fd, addr, 0);
    if (ret != addr) {
        printf ("Seek error: result %06X, expected %06X.\n", ret, addr);
        exit (-1);
    }
    ret = read (fd, &rval, 1);
    if (ret != 1) {
        printf ("Read error at %06X: %s\n", addr, strerror(ret));
        exit (-1);
    }
    if (verbose)
        printf ("    Got %02X.\n", rval);

    if (rval != val)
        printf ("Data error: address %06X written %02X read %02X.\n",
            addr, val, rval);
}

void block_write (unsigned addr, unsigned char *data, unsigned nbytes)
{
    int ret, i;

    if (verbose) {
        printf ("Write block to address %06X.\n", addr);
	printf ("    %02x", data[0]);
	for (i=1; i<nbytes; i++)
            printf ("-%02x", data[i]);
	printf ("\n");
    }
    ret = lseek (fd, addr, 0);
    if (ret != addr) {
        printf ("Seek error at %06X: result %06X, expected %06X.\n",
            addr, ret, addr);
        exit (-1);
    }
    ret = write (fd, data, nbytes);
    if (ret != nbytes) {
        printf ("Write error at %06X: %s\n", addr, strerror(ret));
        exit (-1);
    }
}

void block_read (unsigned addr, unsigned char *data, unsigned nbytes)
{
    int ret, i;

    if (verbose)
        printf ("Read block from address %06X.\n", addr);

    ret = lseek (fd, addr, 0);
    if (ret != addr) {
        printf ("Seek error: result %06X, expected %06X.\n", ret, addr);
        exit (-1);
    }
    ret = read (fd, data, nbytes);
    if (ret != nbytes) {
        printf ("Read error at %06X: %s\n", addr, strerror(ret));
        exit (-1);
    }
    if (verbose) {
	printf ("    %02x", data[0]);
	for (i=1; i<nbytes; i++)
            printf ("-%02x", data[i]);
	printf ("\n");
    }
}

void test_address (unsigned address)
{
    int i;

    printf ("Testing address %06X.\n", address);

    for (i=0; i<8; ++i) {
        byte_write (address, 0x55);
        byte_check (address, 0x55);

        byte_write (address, 0xAA);
        byte_check (address, 0xAA);
    }
    printf ("Done.\n");
}

void test_block (unsigned address)
{
    unsigned char data [256];
    unsigned char rdata [256];
    int i;

    printf ("Testing block at address %06X.\n", address);
    for (i=0; i<sizeof(data); ++i)
        data[i] = i;

    block_write (address, data, sizeof(data));
    block_read (address, rdata, sizeof(rdata));

    for (i=0; i<sizeof(data); ++i) {
        if (rdata[i] != data[i])
            printf ("Data error: offset %d written %02X read %02X.\n",
                i, data[i], rdata[i]);
    }
    printf ("Done.\n");
}

void test_address_bus ()
{
    printf ("Testing address signals.\n");
    byte_write (0x000000, 0x55);
    byte_write (0x000004, 0x11);
    byte_write (0x000008, 0x12);
    byte_write (0x000010, 0x14);
    byte_write (0x000020, 0x18);
    byte_write (0x000040, 0x21);
    byte_write (0x000080, 0x22);
    byte_write (0x000100, 0x24);
    byte_write (0x000200, 0x28);
    byte_write (0x000400, 0x31);
    byte_write (0x000800, 0x32);
    byte_write (0x001000, 0x34);
    byte_write (0x002000, 0x38);
    byte_write (0x004000, 0x41);
    byte_write (0x008000, 0x42);
    byte_write (0x010000, 0x44);
    byte_write (0x020000, 0x48);
    byte_write (0x040000, 0x51);
    byte_write (0x080000, 0x52);
    byte_write (0x100000, 0x54);

    byte_check (0x000000, 0x55);
    byte_check (0x000004, 0x11);
    byte_check (0x000008, 0x12);
    byte_check (0x000010, 0x14);
    byte_check (0x000020, 0x18);
    byte_check (0x000040, 0x21);
    byte_check (0x000080, 0x22);
    byte_check (0x000100, 0x24);
    byte_check (0x000200, 0x28);
    byte_check (0x000400, 0x31);
    byte_check (0x000800, 0x32);
    byte_check (0x001000, 0x34);
    byte_check (0x002000, 0x38);
    byte_check (0x004000, 0x41);
    byte_check (0x008000, 0x42);
    byte_check (0x010000, 0x44);
    byte_check (0x020000, 0x48);
    byte_check (0x040000, 0x51);
    byte_check (0x080000, 0x52);
    byte_check (0x100000, 0x54);
    printf ("Done.\n");
}

void usage ()
{
    fprintf (stderr, "Disk testing utility.\n");
    fprintf (stderr, "Usage:\n");
    fprintf (stderr, "    %s [options] device\n", progname);
    fprintf (stderr, "Options:\n");
    fprintf (stderr, "    -a address      -- test a byte at address,\n");
    fprintf (stderr, "                       write/read 55-AA-55-AA-...\n");
    fprintf (stderr, "    -c address      -- test 256 bytes at address,\n");
    fprintf (stderr, "                       write/read 0-1-2-..-FF\n");
    fprintf (stderr, "    -b              -- test addresses 1, 2, 4, 8... 0x100000\n");
    fprintf (stderr, "    -v              -- verbose mode\n");
    exit (-1);
}

int main (int argc, char **argv)
{
    int aflag = 0, bflag = 0, cflag = 0;
    unsigned address = 0;

    progname = *argv;
    for (;;) {
        switch (getopt (argc, argv, "a:bc:v")) {
        case EOF:
            break;
        case 'a':
            aflag = 1;
            address = strtoul (optarg, 0, 0) % MEM_SIZE;
            continue;
        case 'b':
            bflag = 1;
            continue;
        case 'c':
            cflag = 1;
            address = strtoul (optarg, 0, 0) % MEM_SIZE;
            continue;
        case 'v':
            verbose = 1;
            continue;
        default:
            usage ();
        }
        break;
    }
    argc -= optind;
    argv += optind;
    if (argc != 1 || aflag + bflag + cflag == 0)
        usage ();

    fd = open (argv[0], O_RDWR);
    if (fd < 0) {
        perror (argv[0]);
        return -1;
    }

    if (aflag)
        test_address (address);

    if (bflag)
        test_address_bus();

    if (cflag)
        test_block (address);

    return 0;
}
