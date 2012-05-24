/*
 * Web  Server
 *
 * A simple web server that shows the value of the analog input pins.
 * using an Arduino Wiznet Ethernet shield.
 *
 * 18 Dec 2009 created by David A. Mellis
 *  4 Sep 2010 modified by Tom Igoe
 * 16 Apr 2012 ported to RetroBSD by Serge Vakulenko
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/gpio.h>
#include <wiznet/ethernet.h>

int main()
{
    /* Initialize the Ethernet server library
     * with the IP address and port you want to use
     * (port 80 is default for HTTP). */
    ethernet_init ();
    server_init (80);

    for (;;) {
        /* Listen for incoming clients. */
        client_t client;
        if (! server_available (&client))
            continue;

        /* An http request ends with a blank line. */
        int currentLineIsBlank = 1;

        while (client_connected (&client)) {
            if (client_available (&client)) {
                char c = client_getc (&client);

                /* If you've gotten to the end of the line (received
                 * a newline character) and the line is blank, the http
                 * request has ended, so you can send a reply. */
                if (c == '\n' && currentLineIsBlank) {
                    int fd, pnum, value;

                    /* Send a standard http response header. */
                    client_puts (&client, "HTTP/1.1 200 OK\n");
                    client_puts (&client, "Content-Type: text/html\n\n");

                    /* Output the value of each digital pin. */
                    fd = open ("/dev/porta", O_RDONLY);
                    if (fd < 0) {
                        client_puts (&client, "Failed to open /dev/porta\n");
                        break;
                    }
                    for (pnum = 0; pnum < 6; pnum++) {
                        char buf [80];
                        value = ioctl (fd, GPIO_POLL | GPIO_PORT (pnum), 0);
                        sprintf (buf, "PORT%c = 0x%04x <br />\n", pnum+'A', value);
                        client_puts (&client, buf);
                    }
                    close (fd);
                    break;
                }
                if (c == '\n') {
                    /* You're starting a new line. */
                    currentLineIsBlank = 1;
                }
                else if (c != '\r') {
                    /* You've gotten a character on the current line. */
                    currentLineIsBlank = 0;
                }
            }
        }

        /* Give the web browser time to receive the data. */
        usleep (1000);

        /* Close the connection. */
        client_stop (&client);
    }
}
