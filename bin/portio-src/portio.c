/*
 * Control general purpose i/o pins.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

enum {
    OP_ASSIGN,
    OP_CONF_INPUT,
    OP_CONF_OUTPUT,
    OP_CONF_OPENDRAIN,
    OP_DECONF,
    OP_SET,
    OP_CLEAR,
    OP_INVERT,
    OP_POLL,
};

char *progname;
int verbose;
int fd;

void parse_port (char *arg, int *pnum, int *pmask)
{
    if (*arg >= 'a' && *arg <='z')
        *pnum = *arg - 'a';
    else if (*arg >= 'A' && *arg <='Z')
        *pnum = *arg - 'A';
    else {
        fprintf (stderr, "%s: incorrect port '%s'\n", progname, arg);
        exit (-1);
    }
    *pmask = 0;
    while (*++arg) {
        // TODO
    }
}

int parse_value (char *arg)
{
    if (arg[0] == '0' && arg[1] == 'b') {
        /* Binary value 0bxxx */
        return strtoul (arg+2, 0, 2);
    }
    return strtoul (arg, 0, 0);
}

void do_op (int op, char *port_arg, char *value_arg)
{
    int pnum, pmask, i, value = 0;

    parse_port (port_arg, &pnum, &pmask);
    if (value_arg)
        value = parse_value (value_arg);

    switch (op) {
    case OP_ASSIGN:
        if (pmask == 0xffff) {
            /* All pins */
            if (ioctl (fd, GPIO_STORE | GPIO_PORT (pnum), value) < 0)
                perror ("GPIO_STORE");

        } else if (pmask & (pmask-1)) {
            /* Several pins */
            for (i=1; ! (pmask & i); i<<=1)
                value <<= 1;
            if (ioctl (fd, GPIO_STORE | GPIO_PORT (pnum), value) < 0)
                perror ("GPIO_STORE");

        } else if (value & 1) {
            /* Set single pin */
            if (ioctl (fd, GPIO_SET | GPIO_PORT (pnum), pmask) < 0)
                perror ("GPIO_SET");
        } else {
            /* Clear single pin */
            if (ioctl (fd, GPIO_CLEAR | GPIO_PORT (pnum), pmask) < 0)
                perror ("GPIO_SET");
        }
        break;
    case OP_CONF_INPUT:
        if (ioctl (fd, GPIO_CONFIN | GPIO_PORT (pnum), pmask) < 0)
            perror ("GPIO_CONFIN");
        break;
    case OP_CONF_OUTPUT:
        if (ioctl (fd, GPIO_CONFOUT | GPIO_PORT (pnum), pmask) < 0)
            perror ("GPIO_CONFOUT");
        break;
    case OP_CONF_OPENDRAIN:
        if (ioctl (fd, GPIO_CONFOD | GPIO_PORT (pnum), pmask) < 0)
            perror ("GPIO_CONFOD");
        break;
    case OP_DECONF:
        if (ioctl (fd, GPIO_DECONF | GPIO_PORT (pnum), pmask) < 0)
            perror ("GPIO_DECONF");
        break;
    case OP_SET:
        if (ioctl (fd, GPIO_SET | GPIO_PORT (pnum), pmask) < 0)
            perror ("GPIO_SET");
        break;
    case OP_CLEAR:
        if (ioctl (fd, GPIO_CLEAR | GPIO_PORT (pnum), pmask) < 0)
            perror ("GPIO_CLEAR");
        break;
    case OP_INVERT:
        if (ioctl (fd, GPIO_INVERT | GPIO_PORT (pnum), pmask) < 0)
            perror ("GPIO_INVERT");
        break;
    default:
    case OP_POLL:
        if (ioctl (fd, GPIO_POLL | GPIO_PORT (pnum), &value) < 0)
            perror ("GPIO_POLL");
        printf ("0x%04x\n", value);
        break;
    }
}

void usage ()
{
    fprintf (stderr, "Control gpio pins.\n");
    fprintf (stderr, "Usage:\n");
    fprintf (stderr, "    %s option...\n", progname);
    fprintf (stderr, "Options:\n");
    fprintf (stderr, "    -i ports        configure ports as input\n");
    fprintf (stderr, "    -o ports        configure ports as output\n");
    fprintf (stderr, "    -d ports        configure ports as open-drain output\n");
    fprintf (stderr, "    -x ports        deconfigure ports\n");
    fprintf (stderr, "    -a port value   assign port value\n");
    fprintf (stderr, "    -s ports        set ports to 1\n");
    fprintf (stderr, "    -c ports        clear ports (set to 0)\n");
    fprintf (stderr, "    -r ports        reverse ports (invert)\n");
    fprintf (stderr, "    -g ports        get port values\n");
    fprintf (stderr, "Ports:\n");
    fprintf (stderr, "    a0              single pin\n");
    fprintf (stderr, "    b3-7,11         list of pins\n");
    fprintf (stderr, "    c               same as c0-15\n");
    exit (-1);
}

int main (int argc, char **argv)
{
    register int c, op;
    register char *arg;
    const char *devname = "/dev/porta";

    if (argc <= 1)
        usage();

    fd = open (devname, 1);
    if (fd < 0) {
        perror (devname);
        return -1;
    }

    op = OP_ASSIGN;
    progname = *argv++;
    for (c=1; c<argc; ++c) {
        arg = *argv++;
        if (arg[0] != '-') {
            if (op == OP_ASSIGN) {
                if (++c >= argc || **argv=='-') {
                    fprintf (stderr, "%s: option -a: second argument missed\n", progname);
                    return -1;
                }
                do_op (op, arg, *argv++);
            } else {
                do_op (op, arg, 0);
            }
            continue;
        }
        if (arg[1] == 0 || arg[2] != 0) {
badop:      fprintf (stderr, "%s: unknown option `%s'\n", progname, arg);
            return -1;
        }
        switch (arg[1]) {
        case 'i':               /* configure ports as input */
            op = OP_CONF_INPUT;
            break;
        case 'o':               /* configure ports as output */
            op = OP_CONF_OUTPUT;
            break;
        case 'd':               /* configure ports as open-drain output */
            op = OP_CONF_OPENDRAIN;
            break;
        case 'x':               /* deconfigure ports */
            op = OP_DECONF;
            break;
        case 's':               /* set ports to 1 */
            op = OP_SET;
            break;
        case 'c':               /* clear ports */
            op = OP_CLEAR;
            break;
        case 'r':               /* reverse ports */
            op = OP_INVERT;
            break;
        case 'g':               /* get port values */
            op = OP_POLL;
            break;
        case 'v':               /* verbose mode */
            verbose++;
            break;
        case 'h':               /* help */
            usage();
        default:
            goto badop;
        }
    }
    return 0;
}
