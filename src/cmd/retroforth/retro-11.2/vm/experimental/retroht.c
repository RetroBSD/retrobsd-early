/* Ngaro VM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Copyright (c) 2008 - 2011, Charles Childers
   Copyright (c) 2009 - 2010, Luke Parrish
   Copyright (c) 2010,        Marc Simpson
   Copyright (c) 2010,        Jay Skeer
   Copyright (c) 2011,        Kenneth Keating
   Copyright (c) 2011,        Jorge Acereda
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>

/* Configuration ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

                +---------+---------+---------+
                | 16 bit  | 32 bit  | 64 bit  |
   +------------+---------+---------+---------+
   | IMAGE_SIZE | 32000   | 1000000 | 1000000 |
   +------------+---------+---------+---------+
   | CELL       | int16_t | int32_t | int64_t |
   +------------+---------+---------+---------+

   If memory is tight, cut the MAX_FILE_NAME and MAX_REQUEST_LENGTH.

   You can also cut the ADDRESSES stack size down, but if you have
   heavy nesting or recursion this may cause problems. If you do modify
   it and experience odd problems, try raising it a bit higher.

   Use -DRX16 to select defaults for 16-bit, or -DRX64 to select the
   defaults for 64-bit. Without these, the compiler will generate a
   standard 32-bit VM.
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define CELL            int32_t
#define IMAGE_SIZE      (1024*1024)
#define ADDRESSES          1024
#define STACK_DEPTH         128
#define PORTS                16
#define MAX_FILE_NAME      1024
#define MAX_REQUEST_LENGTH 1024
#define MAX_OPEN_FILES        8
#define LOCAL                 "retroImage"

#ifdef RX64
#undef CELL
#undef LOCAL
#define CELL     int64_t
#define LOCAL    "retroImage64"
#endif

#ifdef RX16
#undef CELL
#undef LOCAL
#undef IMAGE_SIZE
#define CELL        int16_t
#define IMAGE_SIZE  32000
#define LOCAL       "retroImage16"
#endif


enum vm_opcode { VM_NOP, VM_LIT, VM_DUP, VM_DROP,
                 VM_SWAP, VM_PUSH, VM_POP, VM_LOOP,
                 VM_JUMP, VM_RETURN, VM_GT_JUMP, VM_LT_JUMP,
                 VM_NE_JUMP,VM_EQ_JUMP, VM_FETCH, VM_STORE,
                 VM_ADD, VM_SUB, VM_MUL, VM_DIVMOD,
                 VM_AND, VM_OR, VM_XOR, VM_SHL,
                 VM_SHR, VM_ZERO_EXIT, VM_INC, VM_DEC,
                 VM_IN, VM_OUT, VM_WAIT,
                 // These don't appear in the image
                 VM_RESOLVE, VM_CALL, VM_TAILCALL, VM_BYE };

typedef struct {
  uint16_t shadow[IMAGE_SIZE];
  CELL image[IMAGE_SIZE];
  CELL address[ADDRESSES];
  CELL data[STACK_DEPTH];
  CELL ports[PORTS];
  FILE *files[MAX_OPEN_FILES];
  FILE *input[MAX_OPEN_FILES];
  char filename[MAX_FILE_NAME];
  char request[MAX_REQUEST_LENGTH];
  struct termios new_termios, old_termios;
  CELL * sp;
  CELL * rsp;
  CELL ip;
  CELL tos;
  CELL isp;
  CELL shrink;
} VM;

static inline void dbg(const char * fmt, ...) {
  if (0) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
  }
}

#define TOS  vm->tos
#define DROP TOS = *vm->sp--

/* Helper Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static void rxGetString(VM *vm, int starting)
{
  CELL i = 0;
  while(vm->image[starting] && i < MAX_REQUEST_LENGTH)
    vm->request[i++] = (char)vm->image[starting++];
  vm->request[i] = 0;
}

/* Console I/O Support ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static void rxWriteConsole(VM *vm, CELL c) {
  (c > 0) ? putchar((char)c) : printf("\033[2J\033[1;1H");
  /* Erase the previous character if c = backspace */
  if (c == 8) {
    putchar(32);
    putchar(8);
  }
}

static CELL rxReadConsole(VM *vm) {
  CELL c;
  if ((c = getc(vm->input[vm->isp])) == EOF && vm->input[vm->isp] != stdin) {
    fclose(vm->input[vm->isp--]);
    c = 0;
  }
  if (c == EOF && vm->input[vm->isp] == stdin)
    exit(0);
  return c;
}

static void rxIncludeFile(VM *vm, char *s) {
  FILE *file;
  if ((file = fopen(s, "r")))
    vm->input[++vm->isp] = file;
}

static void rxPrepareInput(VM *vm) {
  vm->isp = 0;
  vm->input[vm->isp] = stdin;
}

static void rxPrepareOutput(VM *vm) {
  tcgetattr(0, &vm->old_termios);
  vm->new_termios = vm->old_termios;
  vm->new_termios.c_iflag &= ~(BRKINT+ISTRIP+IXON+IXOFF);
  vm->new_termios.c_iflag |= (IGNBRK+IGNPAR);
  vm->new_termios.c_lflag &= ~(ICANON+ISIG+IEXTEN+ECHO);
  vm->new_termios.c_cc[VMIN] = 1;
  vm->new_termios.c_cc[VTIME] = 0;
  tcsetattr(0, TCSANOW, &vm->new_termios);
}

static void rxRestoreIO(VM *vm) {
  tcsetattr(0, TCSANOW, &vm->old_termios);
}

/* File I/O Support ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static CELL rxGetFileHandle(VM *vm)
{
  CELL i;
  for(i = 1; i < MAX_OPEN_FILES; i++)
    if (vm->files[i] == 0)
      return i;
  return 0;
}

static void rxAddInputSource(VM *vm) {
  CELL name = TOS; DROP;
  rxGetString(vm, name);
  rxIncludeFile(vm, vm->request);
}

static CELL rxOpenFile(VM *vm) {
  CELL slot, mode, name;
  slot = rxGetFileHandle(vm);
  mode = TOS; DROP;
  name = TOS; DROP;
  rxGetString(vm, name);
  if (slot > 0)
  {
    if (mode == 0)  vm->files[slot] = fopen(vm->request, "r");
    if (mode == 1)  vm->files[slot] = fopen(vm->request, "w");
    if (mode == 2)  vm->files[slot] = fopen(vm->request, "a");
    if (mode == 3)  vm->files[slot] = fopen(vm->request, "r+");
  }
  if (vm->files[slot] == NULL)
  {
    vm->files[slot] = 0;
    slot = 0;
  }
  return slot;
}

static CELL rxReadFile(VM *vm) {
  CELL c;
  c = fgetc(vm->files[TOS]);
  DROP;
  return (c == EOF) ? 0 : c;
}

static CELL rxWriteFile(VM *vm) {
  CELL slot, c, r;
  slot = TOS; DROP;
  c = TOS; DROP;
  r = fputc(c, vm->files[slot]);
  return (r == EOF) ? 0 : 1;
}

static CELL rxCloseFile(VM *vm) {
  fclose(vm->files[TOS]);
  vm->files[TOS] = 0;
  DROP;
  return 0;
}

static CELL rxGetFilePosition(VM *vm) {
  CELL slot = TOS; DROP;
  CELL r;
  r = (CELL) ftell(vm->files[slot]);
  return r;
}

static CELL rxSetFilePosition(VM *vm) {
  CELL slot, pos, r;
  slot = TOS; DROP;
  pos  = TOS; DROP;
  r = fseek(vm->files[slot], pos, SEEK_SET);
  return r;
}

static CELL rxGetFileSize(VM *vm) {
  CELL slot, current, r, size;
  slot = TOS; DROP;
  current = ftell(vm->files[slot]);
  r = fseek(vm->files[slot], 0, SEEK_END);
  size = ftell(vm->files[slot]);
  fseek(vm->files[slot], current, SEEK_SET);
  return (r == 0) ? size : 0;
}

static CELL rxDeleteFile(VM *vm) {
  CELL r;
  CELL name = TOS; DROP;
  rxGetString(vm, name);
  r = (unlink(vm->request) == 0) ? -1 : 0;
  return r;
}

static CELL rxLoadImage(VM *vm, const char *image) {
  FILE *fp;
  CELL x = 0;
  if ((fp = fopen(image, "rb")) != NULL) {
    x = fread(vm->image, sizeof(CELL), IMAGE_SIZE, fp);
    fclose(fp);
  }
  return x;
}

static CELL rxSaveImage(VM *vm, char *image) {
  FILE *fp;
  CELL x = 0;
  if ((fp = fopen(image, "wb")) == NULL)
  {
    printf("Unable to save the retroImage!\n");
    rxRestoreIO(vm);
    exit(2);
  }
  x = fwrite(&vm->image, sizeof(CELL),
             vm->shrink? vm->image[3] : IMAGE_SIZE, fp);
  fclose(fp);
  return x;
}

/* Environment Query ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static void rxQueryEnvironment(VM *vm) {
  CELL req, dest;
  char *r;
  req = TOS;  DROP;
  dest = TOS; DROP;
  rxGetString(vm, req);
  r = getenv(vm->request);
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

/* Device I/O Handler ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static void rxDeviceHandler(VM *vm) {
  struct winsize w;
  if (vm->ports[0] != 1) {
    /* Input */
    if (vm->ports[0] == 0 && vm->ports[1] == 1) {
      vm->ports[1] = rxReadConsole(vm);
      vm->ports[0] = 1;
    }

    /* Output (character generator) */
    if (vm->ports[2] == 1) {
      rxWriteConsole(vm, TOS); DROP;
      vm->ports[2] = 0;
      vm->ports[0] = 1;
    }

    /* File IO and Image Saving */
    if (vm->ports[4] != 0) {
      vm->ports[0] = 1;
      switch (vm->ports[4]) {
        case  1: rxSaveImage(vm, vm->filename);
                 vm->ports[4] = 0;
                 break;
        case  2: rxAddInputSource(vm);
                 vm->ports[4] = 0;
                 break;
        case -1: vm->ports[4] = rxOpenFile(vm);
                 break;
        case -2: vm->ports[4] = rxReadFile(vm);
                 break;
        case -3: vm->ports[4] = rxWriteFile(vm);
                 break;
        case -4: vm->ports[4] = rxCloseFile(vm);
                 break;
        case -5: vm->ports[4] = rxGetFilePosition(vm);
                 break;
        case -6: vm->ports[4] = rxSetFilePosition(vm);
                 break;
        case -7: vm->ports[4] = rxGetFileSize(vm);
                 break;
        case -8: vm->ports[4] = rxDeleteFile(vm);
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
        case -5:  vm->ports[5] = vm->sp - vm->data;
                  break;
        case -6:  vm->ports[5] = vm->rsp - vm->address;
                  break;
        case -7:  vm->ports[5] = 0;
                  break;
        case -8:  vm->ports[5] = time(NULL);
                  break;
        case -9:  vm->ports[5] = 0;
                  vm->ip = IMAGE_SIZE - 2;
                  break;
        case -10: vm->ports[5] = 0;
                  rxQueryEnvironment(vm);
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
  }
}

#undef TOS
#undef DROP
#define IP   ip
#define SP   sp
#define RSP  rsp
#define TOS  tos
#define DROP TOS = *(SP--)
#define NOS  *(SP)
#define DUP  do { SP++; NOS=TOS; } while (0)
#define TORS *RSP

#define SKIPNOPS						\
  if (1) do if (vm->image[IP+1]==0) {				\
	IP++;							\
	if (vm->image[IP+1] == 0)				\
	  IP++;							\
      } while (0)

/* The VM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
intptr_t rxTick(VM * vm) {
  register CELL a;
  register CELL b;
  register CELL * sp;
  register CELL * rsp;
  register CELL ip;
  register CELL tos;
  const char * const opcodes[] = {
      &&label_NOP, &&label_LIT, &&label_DUP, &&label_DROP,
      &&label_SWAP, &&label_PUSH, &&label_POP, &&label_LOOP,
      &&label_JUMP, &&label_RETURN, &&label_GT_JUMP, &&label_LT_JUMP,
      &&label_NE_JUMP,&&label_EQ_JUMP, &&label_FETCH, &&label_STORE,
      &&label_ADD, &&label_SUB, &&label_MUL, &&label_DIVMOD,
      &&label_AND, &&label_OR, &&label_XOR, &&label_SHL,
      &&label_SHR, &&label_ZERO_EXIT, &&label_INC, &&label_DEC,
      &&label_IN, &&label_OUT, &&label_WAIT,
      &&label_RESOLVE, &&label_CALL, &&label_TAILCALL, &&label_BYE};
#define OPADDR(x) opcodes[x] - opcodes[VM_NOP]

  if (vm == (void*)0)
    return OPADDR(VM_RESOLVE);
  if (vm == (void*)1)
    return OPADDR(VM_BYE);

#define PRIM(x) label_##x: asm("# " #x "\n")
#define NEXT goto *(opcodes[VM_NOP] + vm->shadow[++IP])
#define SAVEREGS vm->sp = sp; vm->rsp = rsp; vm->ip = ip; vm->tos = tos
#define RESTREGS sp = vm->sp; rsp = vm->rsp; ip = vm->ip; tos = vm->tos

  RESTREGS;

  for (;;) {
    NEXT;

    PRIM(NOP);
    NEXT;

    PRIM(LIT);
    DUP;
    TOS = vm->image[++IP];
    NEXT;

    PRIM(DUP);
    DUP;
    NEXT;

    PRIM(DROP);
    DROP;
    NEXT;

    PRIM(SWAP);
    a = TOS;
    TOS = NOS;
    NOS = a;
    NEXT;

    PRIM(PUSH);
    RSP++;
    TORS = TOS;
    DROP;
    NEXT;

    PRIM(POP);
    DUP;
    TOS = TORS;
    RSP--;
    NEXT;

    PRIM(LOOP);
    IP++;
    if (--TOS > 0)
      IP = vm->image[IP] - 1;
    else
      DROP;
    NEXT;

    PRIM(JUMP);
    IP++;
    IP = vm->image[IP] - 1;
//    SKIPNOPS;
    NEXT;

    PRIM(RETURN);
    IP = TORS;
    RSP--;
//    SKIPNOPS;
    NEXT;

    PRIM(GT_JUMP);
    IP++;
    if(NOS > TOS)
      IP = vm->image[IP] - 1;
    DROP; DROP;
    NEXT;

    PRIM(LT_JUMP);
    IP++;
    if(NOS < TOS)
      IP = vm->image[IP] - 1;
    DROP; DROP;
    NEXT;

    PRIM(NE_JUMP);
    IP++;
    if(TOS != NOS)
      IP = vm->image[IP] - 1;
    DROP; DROP;
    NEXT;

    PRIM(EQ_JUMP);
    IP++;
    if(TOS == NOS)
      IP = vm->image[IP] - 1;
    DROP; DROP;
    NEXT;

    PRIM(FETCH);
    TOS = vm->image[TOS];
    NEXT;

    PRIM(STORE);
    vm->image[TOS] = NOS;
    DROP; DROP;
    NEXT;

    PRIM(ADD);
    NOS += TOS;
    DROP;
    NEXT;

    PRIM(SUB);
    NOS -= TOS;
    DROP;
    NEXT;

    PRIM(MUL);
    NOS *= TOS;
    DROP;
    NEXT;

    PRIM(DIVMOD);
    a = TOS;
    b = NOS;
    TOS = b / a;
    NOS = b % a;
    NEXT;

    PRIM(AND);
    a = TOS;
    b = NOS;
    DROP;
    TOS = a & b;
    NEXT;

    PRIM(OR);
    a = TOS;
    b = NOS;
    DROP;
    TOS = a | b;
    NEXT;

    PRIM(XOR);
    a = TOS;
    b = NOS;
    DROP;
    TOS = a ^ b;
    NEXT;

    PRIM(SHL);
    a = TOS;
    b = NOS;
    DROP;
    TOS = b << a;
    NEXT;

    PRIM(SHR);
    a = TOS;
    DROP;
    TOS >>= a;
    NEXT;

    PRIM(ZERO_EXIT);
    if (TOS == 0) {
      DROP;
      IP = TORS;
      RSP--;
    }
    NEXT;

    PRIM(INC);
    TOS += 1;
    NEXT;

    PRIM(DEC);
    TOS -= 1;
    NEXT;

    PRIM(IN);
    a = TOS;
    TOS = vm->ports[a];
    vm->ports[a] = 0;
    NEXT;

    PRIM(OUT);
    vm->ports[0] = 0;
    vm->ports[TOS] = NOS;
    DROP; DROP;
    NEXT;

    PRIM(WAIT);
    SAVEREGS;
    rxDeviceHandler(vm);
    RESTREGS;
    NEXT;

    PRIM(CALL);
    RSP++;
    TORS = IP;
    IP = vm->image[IP] - 1;
    SKIPNOPS;
    NEXT;

    PRIM(TAILCALL);
    IP = vm->image[IP] - 1;
    SKIPNOPS;
    NEXT;

    PRIM(RESOLVE);
    a = vm->image[IP];
    b = a < VM_RESOLVE? a : VM_CALL;
    if (b == VM_CALL && vm->image[IP+1] == VM_RETURN)
      b = OPADDR(VM_TAILCALL);
    else
      b = OPADDR(b);
    vm->shadow[IP] = b;
    IP--;
    NEXT;
    PRIM(BYE);
    SAVEREGS;
    return 0;
    NEXT;
  }
}

VM * rxNew() {
  return calloc(1, sizeof(VM));
}

void rxDel(VM * vm) {
  free(vm);
}

int rxInit(VM * vm, const char * img, int shrink) {
  CELL offset;
  unsigned i;
  offset = rxTick((VM*)0);
  for (i = 0; i < IMAGE_SIZE - 1; i++)
    vm->shadow[i] = offset;
  vm->shadow[IMAGE_SIZE-1] = rxTick((VM*)1);
  vm->sp = vm->data;
  vm->rsp = vm->address;
  vm->shrink = shrink;
  strcpy(vm->filename, img);
  return rxLoadImage(vm, img);
}

void rxTerm(VM * vm) {
#ifdef NOTYET
  unsigned i;
  for (i = 0; i < MAX_OPEN_FILES; i++) {
    if (vm->files[i])
      fclose(vm->files[i]);
    if (vm->input[i])
      fclose(vm->input[i]);
  }
#endif
}

/* Main ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
int main(int argc, char **argv) {
  unsigned i;
  VM *vm;
  const char * img = LOCAL;
  int shrink = 0;

  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--image"))
      img = argv[++i];
    if (!strcmp(argv[i], "--shrink"))
      shrink = 1;
  }
  vm = rxNew();
  if (rxInit(vm, img, shrink)) {
    rxPrepareInput(vm);
    rxPrepareOutput(vm);
    for (i = 1; i < argc; i++) {
      if (!strcmp(argv[i], "--with"))
        rxIncludeFile(vm, argv[++i]);
    }
    rxTick(vm);
  }
  rxTerm(vm);
  rxRestoreIO(vm);
  rxDel(vm);
  return 0;
}

