/*
 * Chat Server
 *
 * A simple server that distributes any incoming messages to all
 * connected clients.  To use telnet to your device's IP address and type.
 * Using an Arduino Wiznet Ethernet shield.
 *
 * 18 Dec 2009 created by David A. Mellis
 * 10 Aug 2010 modified by Tom Igoe
 * 16 Apr 2012 ported to RetroBSD by Serge Vakulenko
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <wiznet/ethernet.h>

/*
 * Enter a MAC address and IP address for your controller below.
 * The IP address will be dependent on your local network.
 * Gateway and subnet are optional.
 */
unsigned char mac[]     = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
unsigned char ip[]      = { 192,168,1,177 };
unsigned char gateway[] = { 192,168,1,1 };
unsigned char subnet[]  = { 255,255,0,0 };

int got_a_message = 0;  /* whether you got a message from the client yet */

/*
 * Get MAC address from string to byte array.
 */
void parse_mac (char *str)
{
    register unsigned c, val;
    unsigned char *macp = mac;

    do {
        /* Collect number up to ":". */
        val = 0;
        while ((c = (unsigned char) *str)) {
            if (c >= '0' && c <= '9')
                val = (val << 4) + (c - '0');
            else if (c >= 'a' && c <= 'f')
                val = (val << 4) + (c - 'a' + 10);
            else if (c >= 'A' && c <= 'F')
                val = (val << 4) + (c - 'A' + 10);
            else
                break;
            str++;
        }
        *macp++ = val;
    } while (*str++ == ':' && macp < mac + 6);
}

/*
 * Get IP address from string to byte array.
 */
void parse_ip (unsigned char *val, char *str)
{
    unsigned long addr;

    if (! str)
        return;
    addr = inet_addr (str);
    val[0] = addr >> 24;
    val[1] = addr >> 16;
    val[2] = addr >> 8;
    val[3] = addr >> 0;
}

int main()
{
    /* Get parameters from environment */
    parse_mac(getenv("MAC"));
    parse_ip(ip, getenv("IP"));
    parse_ip(gateway, getenv("GATEWAY"));
    parse_ip(subnet, getenv("NETMASK"));

    /* Initialize the ethernet device */
    ethernet_init (mac, ip, gateway, subnet);

    printf("local MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    printf("local IP address %u.%u.%u.%u\n",
        ip[0], ip[1], ip[2], ip[3]);
    printf("gateway %u.%u.%u.%u\n",
        gateway[0], gateway[1], gateway[2], gateway[3]);
    printf("netmask %u.%u.%u.%u\n",
        subnet[0], subnet[1], subnet[2], subnet[3]);

    /* Start listening for clients.
     * Telnet defaults to port 23. */
    server_init (23);

    for (;;) {
        /* Wait for a new client. */
        client_t client;
        if (! server_available (&client))
            continue;

        /* When the client sends the first byte, say hello. */
        if (! got_a_message) {
            printf ("We have a new client\n");
            client_puts (&client, "Hello, client!\n");
            got_a_message = 1;
        }

        /* Read the bytes incoming from the client.
         * Echo the bytes back to the client. */
        char c = client_getc (&client);
        server_putc (c);

        /* Echo the bytes to the server as well. */
        putchar (c);
        fflush (stdout);
    }
}
