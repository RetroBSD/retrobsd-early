/*
 * Basic LoL Shield Test for Duinomite running RetroBSD.
 *
 * Some LoL signals are used for SD card on Duinomite.
 * So I had to modify Duinomite-shield, by cutting signals
 * D10-D11 and connecting them to A2-A5.
 *
 * Writen for the LoL Shield, designed by Jimmie Rodgers:
 * http://jimmieprodgers.com/kits/lolshield/
 *
 * Created by Jimmie Rodgers on 12/30/2009.
 * Adapted from: http://www.arduino.cc/playground/Code/BitMath
 * Ported to RetroBSD by Serge Vakulenko, 01/23/2012.
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Version 3 General Public
 * License as published by the Free Software Foundation;
 * or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

/*
 * Number of control pins for LoL Shield.
 */
#define NPINS   12

/*
 * Sequence of pins to set high during refresh cycle.
 */
static const unsigned high [NPINS] =
{
#define HIGH(p,n) ((p-'a') << 4 | n)
    HIGH('b',9),HIGH('b',8),HIGH('b',7),HIGH('b',6),HIGH('b',12),HIGH('b',11),
    HIGH('e',7),HIGH('e',6),HIGH('e',5),HIGH('e',4),HIGH('e',3), HIGH('e',2),
};

/*
 * Pin data to set low during refresh cycle.
 */
unsigned low [NPINS];

/*
 * Pin masks to set low for a pixel.
 */
#define PB09    (1 << 9)            /* Marked D13 on shield (connect to A5) */
#define PB08    (1 << 8)            /* D12 (connect to A4) */
#define PB07    (1 << 7)            /* D11 (connect to A3) */
#define PB06    (1 << 6)            /* D10 (connect to A2) */
#define PB12    (1 << 12)           /* D9 */
#define PB11    (1 << 11)           /* D8 */
#define PE07    (1 << (16+7))       /* D7 */
#define PE06    (1 << (16+6))       /* D6 */
#define PE05    (1 << (16+5))       /* D5 */
#define PE04    (1 << (16+4))       /* D4 */
#define PE03    (1 << (16+3))       /* D3 */
#define PE02    (1 << (16+2))       /* D2 */

/*
 * Remap pixels to low index and mask.
 */
static const int pixel_map [9*14*2] =
{
    0,PE05, 0,PE06, 0,PE07, 0,PB11, 0, PB12, 0,PB06, 0, PB07, 0,PB08,
            0,PE04, 9,PB09, 0,PE03, 10,PB09, 0,PE02, 11,PB09,
    1,PE05, 1,PE06, 1,PE07, 1,PB11, 1, PB12, 1,PB06, 1, PB07, 1,PB09,
            1,PE04, 9,PB08, 1,PE03, 10,PB08, 1,PE02, 11,PB08,
    2,PE05, 2,PE06, 2,PE07, 2,PB11, 2, PB12, 2,PB06, 2, PB08, 2,PB09,
            2,PE04, 9,PB07, 2,PE03, 10,PB07, 2,PE02, 11,PB07,
    3,PE05, 3,PE06, 3,PE07, 3,PB11, 3, PB12, 3,PB07, 3, PB08, 3,PB09,
            3,PE04, 9,PB06, 3,PE03, 10,PB06, 3,PE02, 11,PB06,
    4,PE05, 4,PE06, 4,PE07, 4,PB11, 4, PB06, 4,PB07, 4, PB08, 4,PB09,
            4,PE04, 9,PB12, 4,PE03, 10,PB12, 4,PE02, 11,PB12,
    5,PE05, 5,PE06, 5,PE07, 5,PB12, 5, PB06, 5,PB07, 5, PB08, 5,PB09,
            5,PE04, 9,PB11, 5,PE03, 10,PB11, 5,PE02, 11,PB11,
    6,PE05, 6,PE06, 6,PB11, 6,PB12, 6, PB06, 6,PB07, 6, PB08, 6,PB09,
            6,PE04, 9,PE07, 6,PE03, 10,PE07, 6,PE02, 11,PE07,
    7,PE05, 7,PE07, 7,PB11, 7,PB12, 7, PB06, 7,PB07, 7, PB08, 7,PB09,
            7,PE04, 9,PE06, 7,PE03, 10,PE06, 7,PE02, 11,PE06,
    8,PE06, 8,PE07, 8,PB11, 8,PB12, 8, PB06, 8,PB07, 8, PB08, 8,PB09,
            8,PE04, 9,PE05, 8,PE03, 10,PE05, 8,PE02, 11,PE05,
};

/*
 * The movie array is what contains the frame data. Each line is one full frame.
 * Since each number is 16 bits, we can easily fit all 14 LEDs per row into it.
 * The number is calculated by adding up all the bits, starting with lowest on
 * the left of each row. -1 was chosen as the kill number, so make sure that
 * is at the end of the matrix, or the program will continue to read into memory.
 *
 * Here PROGMEM is called, which stores the array into ROM, which leaves us
 * with our RAM. You cannot change the array during run-time, only when you
 * upload to the Arduino. You will need to pull it out of ROM, which is covered
 * below. If you want it to stay in RAM, just delete PROGMEM
 */
static const short movie[][9] =
{
// Diagonal swipe across the screen
{1, 0, 0, 0, 0, 0, 0, 0, 0},
{3, 1, 0, 0, 0, 0, 0, 0, 0},
{7, 3, 1, 0, 0, 0, 0, 0, 0},
{15, 7, 3, 1, 0, 0, 0, 0, 0},
{31, 15, 7, 3, 1, 0, 0, 0, 0},
{63, 31, 15, 7, 3, 1, 0, 0, 0},
{127, 63, 31, 15, 7, 3, 1, 0, 0},
{255, 127, 63, 31, 15, 7, 3, 1, 0},
{511, 255, 127, 63, 31, 15, 7, 3, 1},
{1023, 511, 255, 127, 63, 31, 15, 7, 3},
{2047, 1023, 511, 255, 127, 63, 31, 15, 7},
{4095, 2047, 1023, 511, 255, 127, 63, 31, 15},
{8191, 4095, 2047, 1023, 511, 255, 127, 63, 31},
{16383, 8191, 4095, 2047, 1023, 511, 255, 127, 63},
{16383, 16383, 8191, 4095, 2047, 1023, 511, 255, 127},
{16383, 16383, 16383, 8191, 4095, 2047, 1023, 511, 255},
{16383, 16383, 16383, 16383, 8191, 4095, 2047, 1023, 511},
{16383, 16383, 16383, 16383, 16383, 8191, 4095, 2047, 1023},
{16383, 16383, 16383, 16383, 16383, 16383, 8191, 4095, 2047},
{16383, 16383, 16383, 16383, 16383, 16383, 16383, 8191, 4095},
{16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383, 8191},
{16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383},
{16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383},
{16382, 16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383},
{16380, 16382, 16383, 16383, 16383, 16383, 16383, 16383, 16383},
{16376, 16380, 16382, 16383, 16383, 16383, 16383, 16383, 16383},
{16368, 16376, 16380, 16382, 16383, 16383, 16383, 16383, 16383},
{16352, 16368, 16376, 16380, 16382, 16383, 16383, 16383, 16383},
{16320, 16352, 16368, 16376, 16380, 16382, 16383, 16383, 16383},
{16256, 16320, 16352, 16368, 16376, 16380, 16382, 16383, 16383},
{16128, 16256, 16320, 16352, 16368, 16376, 16380, 16382, 16383},
{15872, 16128, 16256, 16320, 16352, 16368, 16376, 16380, 16382},
{15360, 15872, 16128, 16256, 16320, 16352, 16368, 16376, 16380},
{14336, 15360, 15872, 16128, 16256, 16320, 16352, 16368, 16376},
{12288, 14336, 15360, 15872, 16128, 16256, 16320, 16352, 16368},
{8192, 12288, 14336, 15360, 15872, 16128, 16256, 16320, 16352},
{0, 8192, 12288, 14336, 15360, 15872, 16128, 16256, 16320},
{0, 0, 8192, 12288, 14336, 15360, 15872, 16128, 16256},
{0, 0, 0, 8192, 12288, 14336, 15360, 15872, 16128},
{0, 0, 0, 0, 8192, 12288, 14336, 15360, 15872},
{0, 0, 0, 0, 0, 8192, 12288, 14336, 15360},
{0, 0, 0, 0, 0, 0, 8192, 12288, 14336},
{0, 0, 0, 0, 0, 0, 0, 8192, 12288},
{0, 0, 0, 0, 0, 0, 0, 0, 8192},
{0, 0, 0, 0, 0, 0, 0, 0, 0},

// Horizontal swipe
{1, 1, 1, 1, 1, 1, 1, 1, 1} ,
{3, 3, 3, 3, 3, 3, 3, 3, 3},
{7, 7, 7, 7, 7, 7, 7, 7, 7},
{15, 15, 15, 15, 15, 15, 15, 15, 15},
{31, 31, 31, 31, 31, 31, 31, 31, 31},
{63, 63, 63, 63, 63, 63, 63, 63, 63},
{127, 127, 127, 127, 127, 127, 127, 127, 127},
{255, 255, 255, 255, 255, 255, 255, 255, 255},
{511, 511, 511, 511, 511, 511, 511, 511, 511},
{1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023},
{2047, 2047, 2047, 2047, 2047, 2047, 2047, 2047, 2047},
{4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
{8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191},
{16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383},
{16382, 16382, 16382, 16382, 16382, 16382, 16382, 16382, 16382},
{16380, 16380, 16380, 16380, 16380, 16380, 16380, 16380, 16380},
{16376, 16376, 16376, 16376, 16376, 16376, 16376, 16376, 16376},
{16368, 16368, 16368, 16368, 16368, 16368, 16368, 16368, 16368},
{16352, 16352, 16352, 16352, 16352, 16352, 16352, 16352, 16352},
{16320, 16320, 16320, 16320, 16320, 16320, 16320, 16320, 16320},
{16256, 16256, 16256, 16256, 16256, 16256, 16256, 16256, 16256},
{16128, 16128, 16128, 16128, 16128, 16128, 16128, 16128, 16128},
{15872, 15872, 15872, 15872, 15872, 15872, 15872, 15872, 15872},
{15360, 15360, 15360, 15360, 15360, 15360, 15360, 15360, 15360},
{14336, 14336, 14336, 14336, 14336, 14336, 14336, 14336, 14336},
{12288, 12288, 12288, 12288, 12288, 12288, 12288, 12288, 12288},
{8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192},
{0, 0, 0, 0, 0, 0, 0, 0, 0},
{-1}
};

/* Gpio device handle. */
int fd;

/*
 * Initialize LoL shield.
 */
void init()
{
    const char *devname = "/dev/porta";

    /* Open gpio device. */
    fd = open (devname, 1);
    if (fd < 0) {
        perror (devname);
        exit (-1);
    }

    /* Configure control pins as inputs.
     * Port B. */
    ioctl (fd, GPIO_PORT(1) | GPIO_DECONF, ~0);
    ioctl (fd, GPIO_PORT(1) | GPIO_CONFIN,
        PB06 | PB07 | PB08 | PB09 | PB11 | PB12);

    /* Port E. */
    ioctl (fd, GPIO_PORT(4) | GPIO_DECONF, ~0);
    ioctl (fd, GPIO_PORT(4) | GPIO_CONFIN,
        (PE02 | PE03 | PE04 | PE05 | PE06 | PE07) >> 16);
}

/*
 * Display a frame.
 */
void display (const short data[])
{
    unsigned x, y, mask, portnum;
    const int *map;

    /* Convert image to array of pin masks. */
    for (y = 0; y < 9; y++) {
        mask = data [y];
        map = &pixel_map [y*14];
        for (x = 0; x < 14; x++) {
            if ((mask >> x) & 1) {
                low [map[0]] |= map[1];
            }
            map += 2;
        }
    }

    /* Display the image. */
    for (y = 0; y < NPINS; y++) {
        /* Set one pin to high. */
        portnum = high [y] >> 4;
        mask = 1 << (high [y] & 15);
        ioctl (fd, GPIO_PORT(portnum) | GPIO_CONFOUT | GPIO_SET, mask);

        /* Set other pins to low. */
        mask = low [y];
        ioctl (fd, GPIO_PORT(1) | GPIO_CONFOUT | GPIO_CLEAR, mask);
        ioctl (fd, GPIO_PORT(4) | GPIO_CONFOUT | GPIO_CLEAR, mask >> 16);

        /* Pause to make it visible. */
        usleep (10);

        /* Clear line for next loop. */
        low [y] = 0;
    }

    /* Turn off.
     * Set all pins to tristate. */
    ioctl (fd, GPIO_PORT(1) | GPIO_CONFIN,
        PB06 | PB07 | PB08 | PB09 | PB11 | PB12);
    ioctl (fd, GPIO_PORT(4) | GPIO_CONFIN,
        (PE02 | PE03 | PE04 | PE05 | PE06 | PE07) >> 16);
}

int main()
{
    unsigned frame;

    init();
    printf ("LoL Demo ");
    fflush (stdout);

    /* Display a sequence of frames. */
    for (frame = 0; movie[frame][0] >= 0; frame++) {
        printf (".");
        fflush (stdout);

        display (movie [frame]);

    }
    printf (" Done\n");
    return 0;
}
