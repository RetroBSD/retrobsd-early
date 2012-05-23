/*
 * Web client
 *
 * This sketch connects to a website (http://www.google.com)
 * using an Arduino Wiznet Ethernet shield.
 *
 * 18 Dec 2009 created by David A. Mellis
 * 16 Apr 2012 ported to RetroBSD by Serge Vakulenko
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <wiznet/ethernet.h>

/*
 * Enter a MAC address and IP address for your controller below.
 * The IP address will be dependent on your local network.
 */
unsigned char mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

unsigned char ip[] = { 192,168,1,177 };

/*
 * Enter the IP address of the server you're connecting to.
 */
unsigned char server[] = { 173,194,33,104 };    /* Google */

client_t client;

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

    /* Start the Ethernet connection. */
    ethernet_init (mac, ip, 0, 0);

    printf("local MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    printf("local IP address %u.%u.%u.%u\n",
        ip[0], ip[1], ip[2], ip[3]);

    /* Give the Ethernet shield a second to initialize. */
    usleep (1000000);

    /* Initialize the Ethernet client library
     * with the IP address and port of the server
     * that you want to connect to (port 80 is default for HTTP). */
    client_init (&client, server, 80);

    printf("connecting to %u.%u.%u.%u\n",
        server[0], server[1], server[2], server[3]);

    /* if you get a connection, report back via serial */
    if (client_connect (&client)) {
        printf ("connected\n");

        /* Make a HTTP request. */
        client_puts (&client, "GET /search?q=arduino HTTP/1.0\n\n");
    } else {
        /* If you didn't get a connection to the server. */
        printf ("connection failed\n");
    }

    while (client_connected (&client)) {
        /* If there are incoming bytes available
         * from the server, read them and print them. */
        if (client_available (&client)) {
            char c = client_getc (&client);
            putchar(c);
        }
    }

    /* If the server's disconnected, stop the client. */
    printf("\ndisconnecting.\n");
    client_stop (&client);
    return 0;
}
