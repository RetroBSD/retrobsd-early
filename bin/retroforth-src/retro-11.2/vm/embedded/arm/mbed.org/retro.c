/* Ngaro VM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Copyright (c) 2008 - 2011, Charles Childers
   Copyright (c) 2009 - 2010, Luke Parrish
   Copyright (c) 2010,        Marc Simpson
   Copyright (c) 2010,        Jay Skeer
   Copyright (c) 2011,        Kenneth Keating
   Copyright (c) 2011,        Erturk Kocalar
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "mbed.h"
#include <stdint.h>

Serial pc(USBTX, USBRX);
LocalFileSystem local("local");

/* Configuration ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

                +---------+---------+---------+
                | 16 bit  | 32 bit  | 64 bit  |
   +------------+---------+---------+---------+
   | IMAGE_SIZE | 32000   | 1000000 | 1000000 |
   +------------+---------+---------+---------+
   | CELL       | int16_t | int32_t | int64_t |
   +------------+---------+---------+---------+

   If memory is tight, cut the MAX_FILE_NAME and MAX_ENV_QUERY. For
   most purposes, these can be much smaller.

   You can also cut the ADDRESSES stack size down, but if you have
   heavy nesting or recursion this may cause problems. If you do modify
   it and experience odd problems, try raising it a bit higher.
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define CELL          int16_t
#define IMAGE_SIZE      14000              /* This can be raised to 15k */
#define ADDRESSES         256
#define STACK_DEPTH        50
#define PORTS              24
#define MAX_FILE_NAME      48
#define MAX_ENV_QUERY       1
#define MAX_OPEN_FILES      4

typedef struct {
  CELL sp, rsp, ip;
  CELL data[STACK_DEPTH];
  CELL address[ADDRESSES];
  CELL ports[PORTS];
  CELL image[IMAGE_SIZE];
  char filename[MAX_FILE_NAME];
} VM;

/* Macros ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define IP   vm->ip
#define SP   vm->sp
#define RSP  vm->rsp
#define DROP vm->data[SP] = 0; if (--SP < 0) IP = IMAGE_SIZE;
#define TOS  vm->data[SP]
#define NOS  vm->data[SP-1]
#define TORS vm->address[RSP]

enum vm_opcode {VM_NOP, VM_LIT, VM_DUP, VM_DROP, VM_SWAP, VM_PUSH, VM_POP,
                VM_LOOP, VM_JUMP, VM_RETURN, VM_GT_JUMP, VM_LT_JUMP,
                VM_NE_JUMP,VM_EQ_JUMP, VM_FETCH, VM_STORE, VM_ADD,
                VM_SUB, VM_MUL, VM_DIVMOD, VM_AND, VM_OR, VM_XOR, VM_SHL,
                VM_SHR, VM_ZERO_EXIT, VM_INC, VM_DEC, VM_IN, VM_OUT,
                VM_WAIT };
#define NUM_OPS VM_WAIT + 1

/* Console I/O Support ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
FILE *input[MAX_OPEN_FILES];
CELL isp=0;

void dev_putch(CELL c) {
  if (c > 0) {
    putc((char)c, stdout);
    if (c == '\n') {
      putc('\r', stdout);
    }
  }
  else
    printf("\033[2J\033[1;1H");
  /* Erase the previous character if c = backspace */
  if (c == 8) {
    putc(32, stdout);
    putc(8, stdout);
  }
}

CELL dev_getch() {
  CELL c;
  if ((c = getc(input[isp])) == EOF && input[isp] != stdin) {
    fclose(input[isp--]);
    return 0;
  }
  if (c == EOF && input[isp] == stdin)
    exit(0);
  return c;
}

void dev_include(char *s) {
  FILE *file;
  file = fopen(s, "r");
  if (file)
    input[++isp] = file;
}

void dev_init_input() {
  isp = 0;
  input[isp] = stdin;
}

void dev_init_output() {
}

void dev_cleanup() {
}

/* File I/O Support ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
FILE *files[MAX_OPEN_FILES];

CELL file_free()
{
  CELL i;
  for(i = 1; i < MAX_OPEN_FILES; i++)
  {
    if (files[i] == 0)
      return i;
  }
  return 0;
}

void file_add(VM *vm) {
  char s[MAX_FILE_NAME];
  CELL name = TOS; DROP;
  CELL i = 0;
  while(vm->image[name])
    s[i++] = (char)vm->image[name++];
  s[i] = 0;
  dev_include(s);
}

CELL file_handle(VM *vm) {
  CELL slot = file_free();
  CELL mode = TOS; DROP;
  CELL i, address = TOS; DROP;
  char filename[MAX_FILE_NAME];
  for (i = 0; i < MAX_FILE_NAME; i++) {
    filename[i] = vm->image[address+i];
    if (! filename[i]) break;
  }
  if (slot > 0)
  {
    if (mode == 0)  files[slot] = fopen(filename, "r");
    if (mode == 1)  files[slot] = fopen(filename, "w");
    if (mode == 2)  files[slot] = fopen(filename, "a");
    if (mode == 3)  files[slot] = fopen(filename, "r+");
  }
  if (files[slot] == NULL)
  {
    files[slot] = 0;
    slot = 0;
  }
  return slot;
}

CELL file_readc(VM *vm) {
  CELL c = fgetc(files[TOS]); DROP;
  if ( c == EOF )
    return 0;
  else
    return c;
}

CELL file_writec(VM *vm) {
  CELL slot = TOS; DROP;
  CELL c = TOS; DROP;
  CELL r = fputc(c, files[slot]);
  if ( r == EOF )
    return 0;
  else
    return 1;
}

CELL file_closehandle(VM *vm) {
  fclose(files[TOS]);
  files[TOS] = 0;
  DROP;
  return 0;
}

CELL file_getpos(VM *vm) {
  CELL slot = TOS; DROP;
  return (CELL) ftell(files[slot]);
}

CELL file_seek(VM *vm) {
  CELL slot = TOS; DROP;
  CELL pos  = TOS; DROP;
  CELL r = fseek(files[slot], pos, SEEK_SET);
  return r;
}

CELL file_size(VM *vm) {
  CELL slot = TOS; DROP;
  CELL current = ftell(files[slot]);
  CELL r = fseek(files[slot], 0, SEEK_END);
  CELL size = ftell(files[slot]);
  fseek(files[slot], current, SEEK_SET);
  if ( r == 0 )
    return size;
  else
    return 0;
}

CELL file_delete(VM *vm) {
  CELL i, address;
  char filename[MAX_FILE_NAME];
  address = TOS; DROP;
  for (i = 0; i < MAX_FILE_NAME; i++) {
    filename[i] = vm->image[address+i];
    if (! filename[i]) break;
  }
  if (remove(filename) == 0)
    return -1;
  else
    return 0;
}

CELL vm_load_image(VM *vm, char *image) {
  FILE *fp;
  CELL x = -1;

  if ((fp = fopen(image, "rb")) != NULL) {
    x = fread(&vm->image, sizeof(CELL), IMAGE_SIZE, fp);
    fclose(fp);
  }
  return x;
}

CELL vm_save_image(VM *vm, char *image) {
  FILE *fp;
  CELL x = -1;

  if ((fp = fopen(image, "wb")) == NULL)
  {
    fprintf(stderr, "Sorry, but I couldn't open %s\n", image);
    dev_cleanup();
    exit(-1);
  }

  x = fwrite(&vm->image, sizeof(CELL), vm->image[3], fp);
  fclose(fp);

  return x;
}

/* Environment Query ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void dev_getenv(VM *vm) {
  DROP; DROP;
}

/* mbed devices ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  DigitalInOut P5(p5);
  DigitalInOut P6(p6);
  DigitalInOut P7(p7);
  DigitalInOut P8(p8);
  DigitalInOut P9(p9);
  DigitalInOut P10(p10);
  DigitalInOut P11(p11);
  DigitalInOut P12(p12);
  DigitalInOut P13(p13);
  DigitalInOut P14(p14);
  DigitalInOut P15(p15);
  DigitalInOut P16(p16);
  DigitalInOut P17(p17);
  DigitalInOut P18(p18);
  DigitalInOut P19(p19);
  DigitalInOut P20(p20);
  DigitalInOut P21(p21);
  DigitalInOut P22(p22);
  DigitalInOut P23(p23);
  DigitalInOut P24(p24);
  DigitalInOut P25(p25);
  DigitalInOut P26(p26);
  DigitalInOut P27(p27);
  DigitalInOut P28(p28);
  DigitalInOut P29(p29);
  DigitalInOut P30(p30);
  DigitalOut L1(LED1);
  DigitalOut L2(LED2);
  DigitalOut L3(LED3);
  DigitalOut L4(LED4);
void handle_mbed(VM *vm) {
  switch (vm->ports[13]) {
    case -1: switch (TOS) {
               case 5: P5.output(); break;
               case 6: P6.output(); break;
               case 7: P7.output(); break;
               case 8: P8.output(); break;
               case 9: P9.output(); break;
               case 10: P10.output(); break;
               case 11: P11.output(); break;
               case 12: P12.output(); break;
               case 13: P13.output(); break;
               case 14: P14.output(); break;
               case 15: P15.output(); break;
               case 16: P16.output(); break;
               case 17: P17.output(); break;
               case 18: P18.output(); break;
               case 19: P19.output(); break;
               case 20: P20.output(); break;
               case 21: P21.output(); break;
               case 22: P22.output(); break;
               case 23: P23.output(); break;
               case 24: P24.output(); break;
               case 25: P25.output(); break;
               case 26: P26.output(); break;
               case 27: P27.output(); break;
               case 28: P28.output(); break;
               case 29: P29.output(); break;
               case 30: P30.output(); break;
             }
             vm->ports[13] = 0;
             break;
    case -2: switch (TOS) {
               case 5: P5.input(); break;
               case 6: P6.input(); break;
               case 7: P7.input(); break;
               case 8: P8.input(); break;
               case 9: P9.input(); break;
               case 10: P10.input(); break;
               case 11: P11.input(); break;
               case 12: P12.input(); break;
               case 13: P13.input(); break;
               case 14: P14.input(); break;
               case 15: P15.input(); break;
               case 16: P16.input(); break;
               case 17: P17.input(); break;
               case 18: P18.input(); break;
               case 19: P19.input(); break;
               case 20: P20.input(); break;
               case 21: P21.input(); break;
               case 22: P22.input(); break;
               case 23: P23.input(); break;
               case 24: P24.input(); break;
               case 25: P25.input(); break;
               case 26: P26.input(); break;
               case 27: P27.input(); break;
               case 28: P28.input(); break;
               case 29: P29.input(); break;
               case 30: P30.input(); break;
             }
             vm->ports[13] = 0;
             break;
    case  5: P5 = TOS;  DROP; vm->ports[13] = 0; break;
    case  6: P6 = TOS;  DROP; vm->ports[13] = 0; break;
    case  7: P7 = TOS;  DROP; vm->ports[13] = 0; break;
    case  8: P8 = TOS;  DROP; vm->ports[13] = 0; break;
    case  9: P9 = TOS;  DROP; vm->ports[13] = 0; break;
    case 10: P10 = TOS; DROP; vm->ports[13] = 0; break;
    case 11: P11 = TOS; DROP; vm->ports[13] = 0; break;
    case 12: P12 = TOS; DROP; vm->ports[13] = 0; break;
    case 13: P13 = TOS; DROP; vm->ports[13] = 0; break;
    case 14: P14 = TOS; DROP; vm->ports[13] = 0; break;
    case 15: P15 = TOS; DROP; vm->ports[13] = 0; break;
    case 16: P16 = TOS; DROP; vm->ports[13] = 0; break;
    case 17: P17 = TOS; DROP; vm->ports[13] = 0; break;
    case 18: P18 = TOS; DROP; vm->ports[13] = 0; break;
    case 19: P19 = TOS; DROP; vm->ports[13] = 0; break;
    case 20: P20 = TOS; DROP; vm->ports[13] = 0; break;
    case 21: P21 = TOS; DROP; vm->ports[13] = 0; break;
    case 22: P22 = TOS; DROP; vm->ports[13] = 0; break;
    case 23: P23 = TOS; DROP; vm->ports[13] = 0; break;
    case 24: P24 = TOS; DROP; vm->ports[13] = 0; break;
    case 25: P25 = TOS; DROP; vm->ports[13] = 0; break;
    case 26: P26 = TOS; DROP; vm->ports[13] = 0; break;
    case 27: P27 = TOS; DROP; vm->ports[13] = 0; break;
    case 28: P28 = TOS; DROP; vm->ports[13] = 0; break;
    case 29: P29 = TOS; DROP; vm->ports[13] = 0; break;
    case 30: P30 = TOS; DROP; vm->ports[13] = 0; break;
    case 31: L1  = TOS; DROP; vm->ports[13] = 0; break;
    case 32: L2  = TOS; DROP; vm->ports[13] = 0; break;
    case 33: L3  = TOS; DROP; vm->ports[13] = 0; break;
    case 34: L4  = TOS; DROP; vm->ports[13] = 0; break;
    default: vm->ports[13] = 0;
  }
  switch (vm->ports[14]) {
    case  5: SP++; TOS = P5; vm->ports[14] = 0; break;
    case  6: SP++; TOS = P6; vm->ports[14] = 0; break;
    case  7: SP++; TOS = P7; vm->ports[14] = 0; break;
    case  8: SP++; TOS = P8; vm->ports[14] = 0; break;
    case  9: SP++; TOS = P9; vm->ports[14] = 0; break;
    case 10: SP++; TOS = P10; vm->ports[14] = 0; break;
    case 11: SP++; TOS = P11; vm->ports[14] = 0; break;
    case 12: SP++; TOS = P12; vm->ports[14] = 0; break;
    case 13: SP++; TOS = P13; vm->ports[14] = 0; break;
    case 14: SP++; TOS = P14; vm->ports[14] = 0; break;
    case 15: SP++; TOS = P15; vm->ports[14] = 0; break;
    case 16: SP++; TOS = P16; vm->ports[14] = 0; break;
    case 17: SP++; TOS = P17; vm->ports[14] = 0; break;
    case 18: SP++; TOS = P18; vm->ports[14] = 0; break;
    case 19: SP++; TOS = P19; vm->ports[14] = 0; break;
    case 20: SP++; TOS = P20; vm->ports[14] = 0; break;
    case 21: SP++; TOS = P21; vm->ports[14] = 0; break;
    case 22: SP++; TOS = P22; vm->ports[14] = 0; break;
    case 23: SP++; TOS = P23; vm->ports[14] = 0; break;
    case 24: SP++; TOS = P24; vm->ports[14] = 0; break;
    case 25: SP++; TOS = P25; vm->ports[14] = 0; break;
    case 26: SP++; TOS = P26; vm->ports[14] = 0; break;
    case 27: SP++; TOS = P27; vm->ports[14] = 0; break;
    case 28: SP++; TOS = P28; vm->ports[14] = 0; break;
    case 29: SP++; TOS = P29; vm->ports[14] = 0; break;
    case 30: SP++; TOS = P30; vm->ports[14] = 0; break;
    default: vm->ports[14] = 0;
  }
}

/* Device I/O Handler ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void handle_devices(VM *vm) {
  if (vm->ports[0] != 1) {
    /* Input */
    if (vm->ports[0] == 0 && vm->ports[1] == 1) {
      vm->ports[1] = dev_getch();
      vm->ports[0] = 1;
    }

    /* Output (character generator) */
    if (vm->ports[2] == 1) {
      dev_putch(TOS); DROP
      vm->ports[2] = 0;
      vm->ports[0] = 1;
    }

    /* File IO and Image Saving */
    if (vm->ports[4] != 0) {
      vm->ports[0] = 1;
      switch (vm->ports[4]) {
        case  1: vm_save_image(vm, vm->filename);
                 vm->ports[4] = 0;
                 break;
        case  2: file_add(vm);
                 vm->ports[4] = 0;
                 break;
        case -1: vm->ports[4] = file_handle(vm);
                 break;
        case -2: vm->ports[4] = file_readc(vm);
                 break;
        case -3: vm->ports[4] = file_writec(vm);
                 break;
        case -4: vm->ports[4] = file_closehandle(vm);
                 break;
        case -5: vm->ports[4] = file_getpos(vm);
                 break;
        case -6: vm->ports[4] = file_seek(vm);
                 break;
        case -7: vm->ports[4] = file_size(vm);
                 break;
        case -8: vm->ports[4] = file_delete(vm);
                 break;
        default: vm->ports[4] = 0;
      }
    }

    /* Capabilities */
    if (vm->ports[5] != 0) {
      vm->ports[0] = 1;
      switch(vm->ports[5]) {
        case -1:  vm->ports[5] = IMAGE_SIZE;
                  break;
        case -2:  vm->ports[5] = 0;
                  break;
        case -3:  vm->ports[5] = 0;
                  break;
        case -4:  vm->ports[5] = 0;
                  break;
        case -5:  vm->ports[5] = SP;
                  break;
        case -6:  vm->ports[5] = RSP;
                  break;
        case -7:  vm->ports[5] = 0;
                  break;
        case -8:  vm->ports[5] = time(NULL);
                  break;
        case -9:  vm->ports[5] = 0;
                  IP = IMAGE_SIZE;
                  break;
        case -10: vm->ports[5] = 0;
                  dev_getenv(vm);
                  break;
        case -11: vm->ports[5] = 80;
                  break;
        case -12: vm->ports[5] = 25;
                  break;
        default:  vm->ports[5] = 0;
      }
    }

    /* mbed io devices */
    if (vm->ports[13] != 0 || vm->ports[14] != 0) {
      vm->ports[0] = 1;
      handle_mbed(vm);
    }

  }
}

/* The VM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void init_vm(VM *vm) {
   CELL a;
   IP = 0;  SP = 0;  RSP = 0;
   for (a = 0; a < STACK_DEPTH; a++)
      vm->data[a] = 0;
   for (a = 0; a < ADDRESSES; a++)
      vm->address[a] = 0;
   for (a = 0; a < IMAGE_SIZE; a++)
      vm->image[a] = 0;
   for (a = 0; a < PORTS; a++)
      vm->ports[a] = 0;
}

void vm_process(VM *vm) {
  CELL a, b, opcode;
  opcode = vm->image[IP];

  switch(opcode) {
    case VM_NOP:
         break;
    case VM_LIT:
         SP++;
         IP++;
         TOS = vm->image[IP];
         break;
    case VM_DUP:
         SP++;
         vm->data[SP] = NOS;
         break;
    case VM_DROP:
         DROP
         break;
    case VM_SWAP:
         a = TOS;
         TOS = NOS;
         NOS = a;
         break;
    case VM_PUSH:
         RSP++;
         TORS = TOS;
         DROP
         break;
    case VM_POP:
         SP++;
         TOS = TORS;
         RSP--;
         break;
    case VM_LOOP:
         TOS--;
         if (TOS != 0 && TOS > -1)
         {
           IP++;
           IP = vm->image[IP] - 1;
         }
         else
         {
           IP++;
           DROP;
         }
         break;
    case VM_JUMP:
         IP++;
         IP = vm->image[IP] - 1;
         if (IP < 0)
           IP = IMAGE_SIZE;
         else {
           if (vm->image[IP+1] == 0)
             IP++;
           if (vm->image[IP+1] == 0)
             IP++;
         }
         break;
    case VM_RETURN:
         IP = TORS;
         RSP--;
         if (IP < 0)
           IP = IMAGE_SIZE;
         else {
           if (vm->image[IP+1] == 0)
             IP++;
           if (vm->image[IP+1] == 0)
             IP++;
         }
         break;
    case VM_GT_JUMP:
         IP++;
         if(NOS > TOS)
           IP = vm->image[IP] - 1;
         DROP DROP
         break;
    case VM_LT_JUMP:
         IP++;
         if(NOS < TOS)
           IP = vm->image[IP] - 1;
         DROP DROP
         break;
    case VM_NE_JUMP:
         IP++;
         if(TOS != NOS)
           IP = vm->image[IP] - 1;
         DROP DROP
         break;
    case VM_EQ_JUMP:
         IP++;
         if(TOS == NOS)
           IP = vm->image[IP] - 1;
         DROP DROP
         break;
    case VM_FETCH:
         TOS = vm->image[TOS];
         break;
    case VM_STORE:
         vm->image[TOS] = NOS;
         DROP DROP
         break;
    case VM_ADD:
         NOS += TOS;
         DROP
         break;
    case VM_SUB:
         NOS -= TOS;
         DROP
         break;
    case VM_MUL:
         NOS *= TOS;
         DROP
         break;
    case VM_DIVMOD:
         a = TOS;
         b = NOS;
         TOS = b / a;
         NOS = b % a;
         break;
    case VM_AND:
         a = TOS;
         b = NOS;
         DROP
         TOS = a & b;
         break;
    case VM_OR:
         a = TOS;
         b = NOS;
         DROP
         TOS = a | b;
         break;
    case VM_XOR:
         a = TOS;
         b = NOS;
         DROP
         TOS = a ^ b;
         break;
    case VM_SHL:
         a = TOS;
         b = NOS;
         DROP
         TOS = b << a;
         break;
    case VM_SHR:
         a = TOS;
         b = NOS;
         DROP
         TOS = b >>= a;
         break;
    case VM_ZERO_EXIT:
         if (TOS == 0) {
           DROP
           IP = TORS;
           RSP--;
         }
         break;
    case VM_INC:
         TOS += 1;
         break;
    case VM_DEC:
         TOS -= 1;
         break;
    case VM_IN:
         a = TOS;
         TOS = vm->ports[a];
         vm->ports[a] = 0;
         break;
    case VM_OUT:
         vm->ports[0] = 0;
         vm->ports[TOS] = NOS;
         DROP DROP
         break;
    case VM_WAIT:
         handle_devices(vm);
         break;
    default:
         RSP++;
         TORS = IP;
         IP = vm->image[IP] - 1;

         if (IP < 0)
           IP = IMAGE_SIZE;
         else {
           if (vm->image[IP+1] == 0)
             IP++;
           if (vm->image[IP+1] == 0)
             IP++;
         }
         break;
  }
  vm->ports[3] = 1;
}

/* Main ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
int main(int argc, char **argv)
{
  VM static vmm;
  VM *vm = &vmm;
  strcpy(vm->filename, "/local/retroImg");

  while (1) {
    init_vm(vm);
    dev_init_input();

    dev_init_output();

    if (vm_load_image(vm, vm->filename) == -1) {
      dev_cleanup();
      printf("Sorry, unable to find %s\n", vm->filename);
      exit(1);
    }

    for (IP = 0; IP < IMAGE_SIZE; IP++)
      vm_process(vm);

    dev_cleanup();
  }
}

