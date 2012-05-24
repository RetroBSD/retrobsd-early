/*
 * Telnet client
 *
 * This sketch connects to a a telnet server (http://www.google.com)
 * using an Arduino Wiznet Ethernet shield.  You'll need a telnet server
 * to test this with.
 * Processing's ChatServer example (part of the network library) works well,
 * running on port 10002. It can be found as part of the examples
 * in the Processing application, available at http://processing.org/
 *
 * Circuit:
 * - Ethernet shield attached to pins 10, 11, 12, 13
 *
 * 14 Sep 2010 created by Tom Igoe
 * 16 Apr 2012 ported to RetroBSD by Serge Vakulenko
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <wiznet/ethernet.h>

/*
 * IP address of the server to connect to.
 */
unsigned char server[4];

client_t client;

int main (int argc, char **argv)
{
    unsigned addr;

    /* Command argument: IP address of server. */
    if (argc != 2) {
        printf ("Usage: %s <ip-address>\n", argv[0]);
        return -1;
    }
    addr = inet_addr (argv[1]);
    server[0] = addr >> 24;
    server[1] = addr >> 16;
    server[2] = addr >> 8;
    server[3] = addr >> 0;

    /* Start the Ethernet connection. */
    ethernet_init ();

    /* Give the Ethernet shield a second to initialize. */
    usleep (1000000);

    /* Initialize the Ethernet client library
     * with the IP address and port of the server
     * that you want to connect to (port 23 is default for telnet;
     * if you're using Processing's ChatServer, use  port 10002). */
    client_init (&client, server, 10002);

    printf("connecting to %u.%u.%u.%u\n",
        server[0], server[1], server[2], server[3]);

    /* If you get a connection, report back via console. */
    if (client_connect (&client)) {
        printf("connected\n");
    } else {
        /* If you didn't get a connection to the server. */
        printf("connection failed\n");
    }

    while (client_connected (&client)) {

        /* If there are incoming bytes available
         * from the server, read them and print them. */
        if (client_available (&client)) {
            char c = client_getc (&client);
            putchar(c);
        }

        /* As long as there are bytes in the serial queue,
         * read them and send them out the socket if it's open. */
        for (;;) {
            off_t nread;
            char inChar;

            if (ioctl (0, FIONREAD, &nread) < 0 || nread == 0)
                break;

            read(0, &inChar, 1);
            if (client_connected (&client)) {
                client_putc (&client, inChar);
            }
        }
    }

    /* If the server's disconnected, stop the client. */
    printf("\ndisconnecting.\n");
    client_stop (&client);
    return 0;
}
