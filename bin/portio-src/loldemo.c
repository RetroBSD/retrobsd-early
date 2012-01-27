/*
 * Basic LoL Shield Test for Duinomite running RetroBSD.
 *
 * Some LoL signals are used for SD card on Duinomite.
 * So I had to modify Duinomite-shield, by cutting signals
 * D9-D12 and connecting them to A1-A5.
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
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/gpio.h>

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

void demo1 (fd)
{
    unsigned frame;

    printf ("LoL Demo ");
    fflush (stdout);

    /* Process a sequence of frames. */
    for (frame = 0; movie[frame][0] >= 0; frame++) {
        printf (".");
        fflush (stdout);

        /* Display a frame. */
        ioctl (fd, GPIO_LOL | 100, movie [frame]);
    }
    printf (" Done\n");
}

void demo2 (fd)
{
    static unsigned short picture[9];
    int y, frame;

    printf ("LoL Demo ");
    fflush (stdout);

    for (frame = 0; frame<14; frame++) {
        printf (".");
        fflush (stdout);
        memset (picture, 0, sizeof(picture));

        for (y=0; y<9; y++)
            picture[y] |= 1 << frame;

        /* Display a frame. */
        ioctl (fd, GPIO_LOL | 150, picture);
    }
    for (frame = 0; frame<9; frame++) {
        printf ("+");
        fflush (stdout);
        memset (picture, 0, sizeof(picture));

        picture[frame] = (1 << 14) - 1;

        /* Display a frame. */
        ioctl (fd, GPIO_LOL | 150, picture);
    }
    for (frame = 0; frame<9; frame++) {
        printf ("=");
        fflush (stdout);
        memset (picture, 0xFF, sizeof(picture));

        /* Display a frame. */
        ioctl (fd, GPIO_LOL | 250, picture);
        ioctl (fd, GPIO_LOL | 250, picture);
    }
    printf (" Done\n");
}

int main (argc, argv)
    char **argv;
{
    const char *devname = "/dev/porta";
    int fd;

    /* Open gpio device. */
    fd = open (devname, 1);
    if (fd < 0) {
        perror (devname);
        exit (-1);
    }

    if (argc > 1) {
        if (argv[1][0] == '2') {
            demo2 (fd);
            return 0;
        }
    }
    demo1 (fd);
    return 0;
}
