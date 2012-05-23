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
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/gpio.h>
#include <arpa/inet.h>
#include <wiznet/ethernet.h>

/*
 * Enter a MAC address and IP address for your controller below.
 * The IP address will be dependent on your local network.
 */
unsigned char mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

unsigned char ip[] = { 192,168,1,177 };

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

    /* Initialize the Ethernet server library
     * with the IP address and port you want to use
     * (port 80 is default for HTTP). */
    ethernet_init (mac, ip, 0, 0);
    server_init (80);

    printf("local MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    printf("local IP address %u.%u.%u.%u\n",
        ip[0], ip[1], ip[2], ip[3]);

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
