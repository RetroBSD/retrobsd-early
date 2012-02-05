/* Ngaro VM for Arduino boards ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Copyright (c) 2011 - 2012, Oleksandr Kozachuk
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* Configuration ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define CELL            int16_t
#define IMAGE_SIZE        31000
#define IMAGE_CACHE_SIZE    181
#define CHANGE_TABLE_SIZE   457
#define ADDRESSES            64
#define STACK_DEPTH          64
#define PORTS                15
#define STRING_BUFFER_SIZE   32

/* General includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Board specific includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifdef BOARD_mega2560
#define BAUD 38400

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <util/setbaud.h>

#define SPI_PORT     PORTB
#define SPI_DDR      DDRB
#define SPI_CS       PB0
#define SPI_SCK      PB1
#define SPI_MOSI     PB2
#define SPI_MISO     PB3

#define DISPLAY_PORT PORTL
#define DISPLAY_DDR  DDRL
#define DISPLAY_DC   PL1
#define DISPLAY_RST  PL0

#else
#ifdef BOARD_native

#include <termios.h>

#else
#error "Unknown platform or -DBOARD_<type> not defined."
#endif
#endif

/* Opcodes and functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
enum vm_opcode {VM_NOP, VM_LIT, VM_DUP, VM_DROP, VM_SWAP, VM_PUSH, VM_POP,
                VM_LOOP, VM_JUMP, VM_RETURN, VM_GT_JUMP, VM_LT_JUMP,
                VM_NE_JUMP,VM_EQ_JUMP, VM_FETCH, VM_STORE, VM_ADD,
                VM_SUB, VM_MUL, VM_DIVMOD, VM_AND, VM_OR, VM_XOR, VM_SHL,
                VM_SHR, VM_ZERO_EXIT, VM_INC, VM_DEC, VM_IN, VM_OUT,
                VM_WAIT };

static void console_puts(char *s);

/* Change store ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
typedef struct CHANGE_ELEMENT {
    CELL key;
    CELL value;
} change_element_t;

typedef struct CHANGE_TABLE {
    uint8_t full;
    uint8_t size;
    change_element_t *elements;
} change_table_t;

static change_table_t change_table[CHANGE_TABLE_SIZE];
static struct { CELL key; CELL value; } image_cache[IMAGE_CACHE_SIZE];
static CELL _cache_pos_temp;

#define img_put(k, v) _img_put(k, v)
static void _img_put(CELL k, CELL v);
#define img_get(k) \
    ( image_cache[_cache_pos_temp = (k % IMAGE_CACHE_SIZE)].key == k ? \
      image_cache[_cache_pos_temp].value : _img_get(k) )
static CELL _img_get(CELL k);

/* Board and image setup ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifdef BOARD_mega2560

#include "console_mega2560.h"
#include "image.hex.h"
#include "eeprom_atmel.h"

#ifdef DISPLAY_nokia3110
#include "display_nokia3110.h"
#endif

#else
#ifdef BOARD_native

#include "console_native.h"
#define PROGMEM
#define prog_int32_t int32_t
#define prog_int16_t int16_t
#define pgm_read_word(x) (*(x))
#define _SFR_IO8(x) SFR_IO8[x]
static int SFR_IO8[64];
#include "image.hex.h"
#include "eeprom_native.h"

#endif
#endif

/* Macros ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define S_DROP do { data[S_SP] = 0; if (--S_SP < 0) S_IP = IMAGE_SIZE; } while(0)
#define S_TOS  data[S_SP]
#define S_NOS  data[S_SP-1]
#define S_TORS address[S_RSP]

/* Console ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static void console_puts(char *s) {
    for (char *x = s; *x; ++x)
        console_putc(*x);
}

/* Image read and write ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static CELL _img_get(CELL k) {
    uint16_t i = 0, p = k % IMAGE_CACHE_SIZE;
    image_cache[p].key = k;
    change_table_t *tbl = &(change_table[(k % CHANGE_TABLE_SIZE)]);
    for (; i < tbl->full && tbl->elements[i].key != k; ++i);
    if (i == tbl->full)
        return (image_cache[p].value = image_read(k));
    return (image_cache[p].value = tbl->elements[i].value);
}

static void _img_put(CELL k, CELL v) {
    uint16_t i = 0, p = k % IMAGE_CACHE_SIZE;
    change_table_t *tbl;
    if (_img_get(k) == v) return;
    tbl = &(change_table[(k % CHANGE_TABLE_SIZE)]);
    for (; i < tbl->full && tbl->elements[i].key != k; ++i);
    if (i == tbl->full) {
        if (tbl->size == tbl->full) {
            tbl->size += 5;
            tbl->elements = (change_element_t*)realloc(tbl->elements,
                    tbl->size * sizeof(change_element_t));
        }
        tbl->elements[i].key = k; tbl->full += 1;
    }
    tbl->elements[i].value = v;
    image_cache[p].key = k;
    image_cache[p].value = v;
}

void img_string(CELL starting, char *buffer, CELL buffer_len)
{
  CELL i = 0, j = starting;
  for(; i < buffer_len && 0 != (buffer[i] = img_get(j)); ++i, ++j);
  buffer[i] = 0;
}

/* Main ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
int main(void)
{
    size_t j; size_t t;
    register CELL S_SP = 0, S_RSP = 0, S_IP = 0;
    CELL data[STACK_DEPTH];
    CELL address[ADDRESSES];
    CELL ports[PORTS];
    CELL a, b;

#ifndef BOARD_native
    _delay_ms(1000);
#endif

#ifdef DISPLAY_ACTIVATED
    char string_buffer[STRING_BUFFER_SIZE+1];
#endif

    console_prepare();
    console_puts("\nInitialize Ngaro VM.\n\n");

#ifdef DISPLAY_ACTIVATED
    display_init();
    display_clear();
    display_set_xy(0, 0);
#endif

    for (j = 0; j < IMAGE_CACHE_SIZE; ++j) image_cache[j].key = -1;
    for (j = 0; j < STACK_DEPTH; ++j) data[j] = 0;
    for (j = 0; j < ADDRESSES; ++j) address[j] = 0;
    for (j = 0; j < PORTS; ++j) ports[j] = 0;
    for (j = 0; j < CHANGE_TABLE_SIZE; ++j) {
        change_table[j].full = change_table[j].size = 0;
        change_table[j].elements = NULL;
    }


    for (S_IP = 0; S_IP >= 0 && S_IP < IMAGE_SIZE; ++S_IP) {
        switch(img_get(S_IP)) {
            case     VM_NOP: break;
            case     VM_LIT: S_SP++; S_IP++; S_TOS = img_get(S_IP); break;
            case     VM_DUP: S_SP++; S_TOS = S_NOS; break;
            case    VM_DROP: S_DROP; break;
            case    VM_SWAP: a = S_TOS; S_TOS = S_NOS; S_NOS = a; break;
            case    VM_PUSH: S_RSP++; S_TORS = S_TOS; S_DROP; break;
            case     VM_POP: S_SP++; S_TOS = S_TORS; S_RSP--; break;
            case    VM_LOOP: S_TOS--; S_IP++;
                             if (S_TOS != 0 && S_TOS > -1)
                                 S_IP = img_get(S_IP) - 1;
                             else S_DROP;
                             break;
            case    VM_JUMP: S_IP++; S_IP = img_get(S_IP) - 1;
                             if (S_IP < 0) S_IP = IMAGE_SIZE;
                             break;
            case  VM_RETURN: S_IP = S_TORS; S_RSP--;
                             if (S_IP < 0) S_IP = IMAGE_SIZE;
                             break;
            case VM_GT_JUMP: S_IP++;
                             if (S_NOS > S_TOS) S_IP = img_get(S_IP) - 1;
                             S_DROP; S_DROP;
                             break;
            case VM_LT_JUMP: S_IP++;
                             if (S_NOS < S_TOS) S_IP = img_get(S_IP) - 1;
                             S_DROP; S_DROP;
                             break;
            case VM_NE_JUMP: S_IP++;
                             if (S_TOS != S_NOS) S_IP = img_get(S_IP) - 1;
                             S_DROP; S_DROP;
                             break;
            case VM_EQ_JUMP: S_IP++;
                             if (S_TOS == S_NOS) S_IP = img_get(S_IP) - 1;
                             S_DROP; S_DROP;
                             break;
            case   VM_FETCH: S_TOS = img_get(S_TOS); break;
            case   VM_STORE: img_put(S_TOS, S_NOS); S_DROP; S_DROP; break;
            case     VM_ADD: S_NOS += S_TOS; S_DROP; break;
            case     VM_SUB: S_NOS -= S_TOS; S_DROP; break;
            case     VM_MUL: S_NOS *= S_TOS; S_DROP; break;
            case  VM_DIVMOD: a = S_TOS; b = S_NOS; S_TOS = b / a; S_NOS = b % a; break;
            case     VM_AND: a = S_TOS; b = S_NOS; S_DROP; S_TOS = a & b; break;
            case      VM_OR: a = S_TOS; b = S_NOS; S_DROP; S_TOS = a | b; break;
            case     VM_XOR: a = S_TOS; b = S_NOS; S_DROP; S_TOS = a ^ b; break;
            case     VM_SHL: a = S_TOS; b = S_NOS; S_DROP; S_TOS = b << a; break;
            case     VM_SHR: a = S_TOS; S_DROP; S_TOS >>= a; break;
            case VM_ZERO_EXIT: if (S_TOS == 0) { S_DROP; S_IP = S_TORS; S_RSP--; } break;
            case     VM_INC: S_TOS += 1; break;
            case     VM_DEC: S_TOS -= 1; break;
            case      VM_IN: a = S_TOS; S_TOS = ports[a]; ports[a] = 0; break;
            case     VM_OUT: t = (size_t) S_TOS; ports[0] = 0; ports[t] = S_NOS; S_DROP; S_DROP; break;
            case    VM_WAIT:
                if (ports[0] != 1) {
                    /* Input */
                    if (ports[0] == 0 && ports[1] == 1) {
                        ports[1] = console_getc();
                        ports[0] = 1;
                    }
                    /* Output (character generator) */
                    if (ports[2] == 1) {
                        console_putc(S_TOS); S_DROP;
                        ports[2] = 0;
                        ports[0] = 1;
                    }
                    /* File IO and Image Saving */
                    if (ports[4] != 0) {
                        ports[0] = 1;
                        switch (ports[4]) {
                            case 1: ports[4] = save_to_eeprom(); break;
                            case 2: ports[4] = load_from_eeprom(); break;
                            default: ports[4] = 0;
                        }
                    }
                    /* Capabilities */
                    if (ports[5] != 0) {
                        ports[0] = 1;
                        switch(ports[5]) {
                            case -1:  ports[5] = IMAGE_SIZE; break;
                            case -5:  ports[5] = S_SP; break;
                            case -6:  ports[5] = S_RSP; break;
                            case -9:  ports[5] = 0; S_IP = IMAGE_SIZE; break;
                            case -10: ports[5] = 0; S_DROP;
                                      img_put(S_TOS, 0); S_DROP;
                                      break;
                            case -11: ports[5] = 80; break;
                            case -12: ports[5] = 25; break;
                            default:  ports[5] = 0;
                        }
                    }
                    /* Arduino ports */
                    if (ports[13] != 0) {
                        ports[0] = 1;
                        switch(ports[13]) {
                            /* IO8 Ports */
                            case -1: ports[13] = _SFR_IO8(S_TOS); S_DROP; break;
                            case -2:
                                _SFR_IO8(S_TOS) = S_NOS;
                                ports[13] = 0; S_DROP; S_DROP;
                                break;
                            case -3:
                                a = S_TOS; b = S_NOS; S_DROP; S_DROP;
                                ports[13] = ((_SFR_IO8(a) & (1 << b)) != 0);
                                break;
                            case -4:
                                a = S_TOS; b = S_NOS; S_DROP; S_DROP;
                                if (S_TOS) _SFR_IO8(a) |= (1 << b);
                                else _SFR_IO8(a) &= ~(1 << b);
                                ports[13] = 0;
                                S_DROP; break;
#ifndef BOARD_native
                            case -5: a = S_TOS; S_DROP;
                                     _delay_ms(a);
                                     ports[13] = 0;
                                     break;
#endif
                            default: ports[13] = 0;
                        }
                    }
                    if (ports[14] != 0) {
                        ports[0] = 1;
#ifdef DISPLAY_ACTIVATED
                        switch(ports[14]) {
                            case -1: display_write_char(S_TOS, S_NOS);
                                     ports[14] = 0;
                                     S_DROP; S_DROP; break;
                            case -2: img_string(S_TOS, string_buffer, STRING_BUFFER_SIZE);
                                     display_write_string(string_buffer, S_NOS);
                                     ports[14] = 0;
                                     S_DROP; S_DROP; break;
                            case -3: display_set_xy(S_NOS, S_TOS);
                                     ports[14] = 0;
                                     S_DROP; S_DROP; break;
                            case -4: display_clear();
                                     ports[14] = 0;
                                     break;
                            case -5: ports[14] = DISPLAY_WIDTH; break;
                            case -6: ports[14] = DISPLAY_HEIGHT; break;
                            default: ports[14] = 0;
                        }
#else
                        switch(ports[14]) {
                            case -1: ports[14] = 0; S_DROP; S_DROP; break;
                            case -2: ports[14] = 0; S_DROP; S_DROP; break;
                            case -3: ports[14] = 0; S_DROP; S_DROP; break;
                            default: ports[14] = 0;
                        }
#endif
                    }
                }
                break;
            default:
                S_RSP++;
                S_TORS = S_IP;
                S_IP = img_get(S_IP) - 1;
                break;
        }
        ports[3] = 1;
    }

    console_puts("\n\nNgaro VM is down.\n");
    console_finish();
#ifndef BOARD_native
    while(1) _delay_ms(100);
#endif
    return 0;
}
