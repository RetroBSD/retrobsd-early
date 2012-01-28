/* Ngaro VM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Copyright (c) 2008 - 2011, Charles Childers
   Copyright (c) 2009 - 2010, Luke Parrish
   Copyright (c) 2010,        Marc Simpson
   Copyright (c) 2010,        Jay Skeer
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

/* Configuration ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define IMAGE_SIZE   1000000
#define ADDRESS_SIZE    1024
#define DATA_SIZE        100
#define GLOBAL       "/usr/local/share/retro/retroImage"

typedef struct {
  int sp, rsp, ip;
  int *data, *address, *image, ports[1024];
  int size_image, size_data, size_address;
  int shrink, padding;
  char filename[2048];
} VM;

/* Macros ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define CELL int
#define DROP vm->data[vm->sp] = 0; if (--vm->sp < 0) vm->ip = vm->size_image;
#define TOS  vm->data[vm->sp]
#define NOS  vm->data[vm->sp-1]
#define TORS vm->address[vm->rsp]

enum vm_opcode {VM_NOP, VM_LIT, VM_DUP, VM_DROP, VM_SWAP, VM_PUSH, VM_POP,
                VM_LOOP, VM_JUMP, VM_RETURN, VM_GT_JUMP, VM_LT_JUMP,
                VM_NE_JUMP,VM_EQ_JUMP, VM_FETCH, VM_STORE, VM_ADD,
                VM_SUB, VM_MUL, VM_DIVMOD, VM_AND, VM_OR, VM_XOR, VM_SHL,
                VM_SHR, VM_ZERO_EXIT, VM_INC, VM_DEC, VM_IN, VM_OUT,
                VM_WAIT };
#define NUM_OPS VM_WAIT + 1

int stats[NUM_OPS + 1];

/* Console I/O Support ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
struct termios new_termios, old_termios;
FILE *input[12];
int isp=0;

void dev_putch(int c) {
  if (c > 0)
    putchar((char)c);
  else
    printf("\033[2J\033[1;1H");
  /* Erase the previous character if c = backspace */
  if (c == 8) {
    putchar(32);
    putchar(8);
  }
}

int dev_getch() {
  int c;
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
  tcgetattr(0, &old_termios);
  new_termios = old_termios;
  new_termios.c_iflag &= ~(BRKINT+ISTRIP+IXON+IXOFF);
  new_termios.c_iflag |= (IGNBRK+IGNPAR);
  new_termios.c_lflag &= ~(ICANON+ISIG+IEXTEN+ECHO);
  new_termios.c_cc[VMIN] = 1;
  new_termios.c_cc[VTIME] = 0;
  tcsetattr(0, TCSANOW, &new_termios);
}

void dev_cleanup() {
  tcsetattr(0, TCSANOW, &old_termios);
}

/* File I/O Support ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
FILE *files[8];

int file_free()
{
  int i;
  for(i = 1; i < 8; i++)
  {
    if (files[i] == 0)
      return i;
  }
  return 0;
}

void file_add(VM *vm) {
  char s[1024];
  int name = TOS; DROP;
  int i = 0;
  while(vm->image[name])
    s[i++] = (char)vm->image[name++];
  s[i] = 0;
  dev_include(s);
}

int file_handle(VM *vm) {
  int slot = file_free();
  int mode = TOS; DROP;
  int i, address = TOS; DROP;
  char filename[256];
  for (i = 0; i < 256; i++) {
    filename[i] = vm->image[address+i];
    if (! filename[i]) break;
  }
  if (slot > 0)
  {
    if (mode == 0)
      files[slot] = fopen(filename, "r");
    if (mode == 1)
      files[slot] = fopen(filename, "w");
    if (mode == 2)
      files[slot] = fopen(filename, "a");
    if (mode == 3)
      files[slot] = fopen(filename, "r+");
  }
  if (files[slot] == NULL)
  {
    files[slot] = 0;
    slot = -1;
  }
  return slot;
}

int file_readc(VM *vm) {
  int c = fgetc(files[TOS]); DROP;
  if ( c == EOF )
    return 0;
  else
    return c;
}

int file_writec(VM *vm) {
  int slot = TOS; DROP;
  int c = TOS; DROP;
  int r = fputc(c, files[slot]);
  if ( r == EOF )
    return 0;
  else
    return 1;
}

int file_closehandle(VM *vm) {
  fclose(files[TOS]);
  files[TOS] = 0;
  DROP;
  return 1;
}

int file_getpos(VM *vm) {
  int slot = TOS; DROP;
  return (int) ftell(files[slot]);
}

int file_seek(VM *vm) {
  int slot = TOS; DROP;
  int pos  = TOS; DROP;
  int r = fseek(files[slot], pos, SEEK_SET);
  return r;
}

int file_size(VM *vm) {
  int slot = TOS; DROP;
  int current = ftell(files[slot]);
  int r = fseek(files[slot], 0, SEEK_END);
  int size = ftell(files[slot]);
  fseek(files[slot], current, SEEK_SET);
  if ( r == 0 )
    return size;
  else
    return 0;
}

int file_delete(VM *vm) {
  int i, address;
  char filename[256];
  address = TOS; DROP;
  for (i = 0; i < 256; i++) {
    filename[i] = vm->image[address+i];
    if (! filename[i]) break;
  }
  return unlink(filename);
}

int vm_load_image(VM *vm, char *image) {
  FILE *fp;
  int x = -1;

  if ((fp = fopen(image, "rb")) != NULL) {
    x = fread((void *)vm->image, sizeof(int), vm->size_image, fp);
    fclose(fp);
  }

  if (x == -1)
  {
    if ((fp = fopen(GLOBAL, "rb")) != NULL) {
      x = fread(&vm->image, sizeof(int), vm->size_image, fp);
      fclose(fp);
    }
  }

  return x;
}

int vm_save_image(VM *vm, char *image) {
  FILE *fp;
  int x = -1;

  if ((fp = fopen(image, "wb")) == NULL)
  {
    fprintf(stderr, "Sorry, but I couldn't open %s\n", image);
    dev_cleanup();
    exit(-1);
  }

  if (vm->shrink == 0)
    x = fwrite((void *)vm->image, sizeof(int), vm->size_image, fp);
  else
    x = fwrite((void *)vm->image, sizeof(int), vm->image[3], fp);
  fclose(fp);

  return x;
}

/* Environment Query ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void dev_getenv(VM *vm) {
  int req, dest, c;
  char s[65535];
  char *r;

  req = TOS;  DROP;
  dest = TOS; DROP;

  for (c = 0; c < 65535; c++)
    s[c] = '\0';
  for (c = 0; vm->image[req] != 0; c++, req++)
    s[c] = (char)vm->image[req];

  r = getenv(s);

  if (r != 0)
    while (*r != '\0')
    {
      vm->image[dest] = *r;
      dest++;
      r++;
    }
  else
    vm->image[dest] = 0;
}

/* The VM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void init_vm(VM *vm, int ram, int data, int address, int shrink) {
   vm->image   = (int *)calloc(ram, sizeof(int));
   vm->data    = (int *)calloc(data, sizeof(int));
   vm->address = (int *)calloc(address, sizeof(int));
   vm->ip = 0;  vm->sp = 0;  vm->rsp = 0;
   vm->size_image   = ram;
   vm->size_data    = data;
   vm->size_address = address;
   vm->shrink       = shrink;
}

void vm_process(VM *vm) {
  struct winsize w;
  int a, b, opcode;
  opcode = vm->image[vm->ip];

  switch(opcode) {
    case VM_NOP:
         stats[VM_NOP]++;
         break;
    case VM_LIT:
         stats[VM_LIT]++;
         vm->sp++;
         vm->ip++;
         TOS = vm->image[vm->ip];
         break;
    case VM_DUP:
         stats[VM_DUP]++;
         vm->sp++;
         vm->data[vm->sp] = NOS;
         break;
    case VM_DROP:
         stats[VM_DROP]++;
         DROP
         break;
    case VM_SWAP:
         stats[VM_SWAP]++;
         a = TOS;
         TOS = NOS;
         NOS = a;
         break;
    case VM_PUSH:
         stats[VM_PUSH]++;
         vm->rsp++;
         TORS = TOS;
         DROP
         break;
    case VM_POP:
         stats[VM_POP]++;
         vm->sp++;
         TOS = TORS;
         vm->rsp--;
         break;
    case VM_LOOP:
         stats[VM_LOOP]++;
         TOS--;
         if (TOS != 0 && TOS > -1)
         {
           vm->ip++;
           vm->ip = vm->image[vm->ip] - 1;
         }
         else
         {
           vm->ip++;
           DROP;
         }
         break;
    case VM_JUMP:
         stats[VM_JUMP]++;
         vm->ip++;
         vm->ip = vm->image[vm->ip] - 1;
         if (vm->ip < 0)
           vm->ip = vm->size_image;
         else {
           if (vm->image[vm->ip+1] == 0)
             vm->ip++;
           if (vm->image[vm->ip+1] == 0)
             vm->ip++;
         }
         break;
    case VM_RETURN:
         stats[VM_RETURN]++;
         vm->ip = TORS;
         vm->rsp--;
         break;
    case VM_GT_JUMP:
         stats[VM_GT_JUMP]++;
         vm->ip++;
         if(NOS > TOS)
           vm->ip = vm->image[vm->ip] - 1;
         DROP DROP
         break;
    case VM_LT_JUMP:
         stats[VM_LT_JUMP]++;
         vm->ip++;
         if(NOS < TOS)
           vm->ip = vm->image[vm->ip] - 1;
         DROP DROP
         break;
    case VM_NE_JUMP:
         stats[VM_NE_JUMP]++;
         vm->ip++;
         if(TOS != NOS)
           vm->ip = vm->image[vm->ip] - 1;
         DROP DROP
         break;
    case VM_EQ_JUMP:
         stats[VM_EQ_JUMP]++;
         vm->ip++;
         if(TOS == NOS)
           vm->ip = vm->image[vm->ip] - 1;
         DROP DROP
         break;
    case VM_FETCH:
         stats[VM_FETCH]++;
         TOS = vm->image[TOS];
         break;
    case VM_STORE:
         stats[VM_STORE]++;
         vm->image[TOS] = NOS;
         DROP DROP
         break;
    case VM_ADD:
         stats[VM_ADD]++;
         NOS += TOS;
         DROP
         break;
    case VM_SUB:
         stats[VM_SUB]++;
         NOS -= TOS;
         DROP
         break;
    case VM_MUL:
         stats[VM_MUL]++;
         NOS *= TOS;
         DROP
         break;
    case VM_DIVMOD:
         stats[VM_DIVMOD]++;
         a = TOS;
         b = NOS;
         TOS = b / a;
         NOS = b % a;
         break;
    case VM_AND:
         stats[VM_AND]++;
         a = TOS;
         b = NOS;
         DROP
         TOS = a & b;
         break;
    case VM_OR:
         stats[VM_OR]++;
         a = TOS;
         b = NOS;
         DROP
         TOS = a | b;
         break;
    case VM_XOR:
         stats[VM_XOR]++;
         a = TOS;
         b = NOS;
         DROP
         TOS = a ^ b;
         break;
    case VM_SHL:
         stats[VM_SHL]++;
         a = TOS;
         b = NOS;
         DROP
         TOS = b << a;
         break;
    case VM_SHR:
         stats[VM_SHR]++;
         a = TOS;
         b = NOS;
         DROP
         TOS = b >>= a;
         break;
    case VM_ZERO_EXIT:
         stats[VM_ZERO_EXIT]++;
         if (TOS == 0) {
           DROP
           vm->ip = TORS;
           vm->rsp--;
         }
         break;
    case VM_INC:
         stats[VM_INC]++;
         TOS += 1;
         break;
    case VM_DEC:
         stats[VM_DEC]++;
         TOS -= 1;
         break;
    case VM_IN:
         stats[VM_IN]++;
         a = TOS;
         TOS = vm->ports[a];
         vm->ports[a] = 0;
         break;
    case VM_OUT:
         stats[VM_OUT]++;
         vm->ports[0] = 0;
         vm->ports[TOS] = NOS;
         DROP DROP
         break;
    case VM_WAIT:
         stats[VM_WAIT]++;
         if (vm->ports[0] == 1)
           break;

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
             case -1:  vm->ports[5] = vm->size_image;
                       break;
             case -2:  vm->ports[5] = 0;
                       break;
             case -3:  vm->ports[5] = 0;
                       break;
             case -4:  vm->ports[5] = 0;
                       break;
             case -5:  vm->ports[5] = vm->sp;
                       break;
             case -6:  vm->ports[5] = vm->rsp;
                       break;
             case -7:  vm->ports[5] = 0;
                       break;
             case -8:  vm->ports[5] = time(NULL);
                       break;
             case -9:  vm->ports[5] = 0;
                       vm->ip = vm->size_image;
                       break;
             case -10: vm->ports[5] = 0;
                       dev_getenv(vm);
                       break;
             case -11: ioctl(0, TIOCGWINSZ, &w);
                       vm->ports[5] = w.ws_col;
                       break;
             case -12: ioctl(0, TIOCGWINSZ, &w);
                       vm->ports[5] = w.ws_row;
                       break;
             default:  vm->ports[5] = 0;
           }
         }
         break;
    default:
         stats[NUM_OPS]++;
         vm->rsp++;
         TORS = vm->ip;
         vm->ip = vm->image[vm->ip] - 1;

         if (vm->ip < 0)
           vm->ip = vm->size_image;
         else {
           if (vm->image[vm->ip+1] == 0)
             vm->ip++;
           if (vm->image[vm->ip+1] == 0)
             vm->ip++;
         }
         break;
  }
  vm->ports[3] = 1;
}

/* Main ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
int main(int argc, char **argv)
{
  int i, s, ram, data, address, shrink;
  VM *vm = malloc(sizeof(VM));

  ram     = IMAGE_SIZE;
  data    = DATA_SIZE;
  address = ADDRESS_SIZE;
  shrink  = 0;

  strcpy(vm->filename, "retroImage");

  dev_init_input();

  s = 0;
  for (i = 0; i < NUM_OPS; i++)
    stats[i] = 0;

  /* Parse the command line arguments */
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--with") == 0)
      dev_include(argv[++i]);
    if (strcmp(argv[i], "--image") == 0)
      strcpy(vm->filename, argv[++i]);
    if (strcmp(argv[i], "--shrink") == 0)
      shrink = 1;
    if (strcmp(argv[i], "--track") == 0)
      s = -1;
    if (strcmp(argv[i], "--ram") == 0)
      ram = strtol(argv[++i], 0, 10);
    if (strcmp(argv[i], "--ds") == 0)
      data = strtol(argv[++i], 0, 10);
    if (strcmp(argv[i], "--as") == 0)
      address = strtol(argv[++i], 0, 10);
  }

  init_vm(vm, ram, data, address, shrink);
  dev_init_output();

  if (vm_load_image(vm, vm->filename) == -1) {
    dev_cleanup();
    printf("Sorry, unable to find %s\n", vm->filename);
    exit(1);
  }

  for (vm->ip = 0; vm->ip < vm->size_image; vm->ip++)
    vm_process(vm);

  dev_cleanup();
  if (s == -1)
  {
    printf("Runtime Statistics\n");
    printf("NOP:     %d\n", stats[VM_NOP]);
    printf("LIT:     %d\n", stats[VM_LIT]);
    printf("DUP:     %d\n", stats[VM_DUP]);
    printf("DROP:    %d\n", stats[VM_DROP]);
    printf("SWAP:    %d\n", stats[VM_SWAP]);
    printf("PUSH:    %d\n", stats[VM_PUSH]);
    printf("POP:     %d\n", stats[VM_POP]);
    printf("LOOP:    %d\n", stats[VM_LOOP]);
    printf("JUMP:    %d\n", stats[VM_JUMP]);
    printf("RETURN:  %d\n", stats[VM_RETURN]);
    printf(">JUMP:   %d\n", stats[VM_GT_JUMP]);
    printf("<JUMP:   %d\n", stats[VM_LT_JUMP]);
    printf("!JUMP:   %d\n", stats[VM_NE_JUMP]);
    printf("=JUMP:   %d\n", stats[VM_EQ_JUMP]);
    printf("FETCH:   %d\n", stats[VM_FETCH]);
    printf("STORE:   %d\n", stats[VM_STORE]);
    printf("ADD:     %d\n", stats[VM_ADD]);
    printf("SUB:     %d\n", stats[VM_SUB]);
    printf("MUL:     %d\n", stats[VM_MUL]);
    printf("DIVMOD:  %d\n", stats[VM_DIVMOD]);
    printf("AND:     %d\n", stats[VM_AND]);
    printf("OR:      %d\n", stats[VM_OR]);
    printf("XOR:     %d\n", stats[VM_XOR]);
    printf("SHL:     %d\n", stats[VM_SHL]);
    printf("SHR:     %d\n", stats[VM_SHR]);
    printf("0;:      %d\n", stats[VM_ZERO_EXIT]);
    printf("INC:     %d\n", stats[VM_INC]);
    printf("DEC:     %d\n", stats[VM_DEC]);
    printf("IN:      %d\n", stats[VM_IN]);
    printf("OUT:     %d\n", stats[VM_OUT]);
    printf("WAIT:    %d\n", stats[VM_WAIT]);
    printf("CALL:    %d\n", stats[NUM_OPS]);

    i = 0;
    for (s = 0; s < NUM_OPS; s++)
    {
      i += stats[s];
    }
    printf("Total opcodes processed: %d\n", i);
  }
  return 0;
}
