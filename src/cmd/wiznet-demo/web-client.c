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
#include <unistd.h>
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

int main()
{
    /* Start the Ethernet connection. */
    ethernet_init (mac, ip, 0, 0);

    /* Give the Ethernet shield a second to initialize. */
    usleep (1000000);

    /* Initialize the Ethernet client library
     * with the IP address and port of the server
     * that you want to connect to (port 80 is default for HTTP). */
    client_init (&client, server, 80);

    printf("connecting...\n");

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
