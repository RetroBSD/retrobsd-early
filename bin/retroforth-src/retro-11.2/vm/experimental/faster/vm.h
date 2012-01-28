#ifndef NGARO_HEADER
#define NGARO_HEADER

#define INPUT  1
#define OUTPUT 2

#define CELL int
#define DROP vm->acc = vm->data[vm->sp]; vm->sp--;
#define TOS  vm->data[vm->sp]
#define NOS  vm->data[vm->sp-1]

#define TORS vm->address[vm->rsp]
#define NORS vm->address[vm->rsp-1]

#define VMOP vm->image[vm->ip]

#define IMAGE_SIZE   1000000
#define STACK_DEPTH      100
#define ADDRESSES       1024
#define COMP_BUFFER  0xFFFFF

typedef struct
{
  int sp;
  int rsp;
  int ip;
  int acc;
  int shrink;
  int data[STACK_DEPTH];
  int address[ADDRESSES];
  int ports[1024];
  FILE *files[12];
  char request[1024];
  int image[IMAGE_SIZE];
  int padding;
  char filename[2048];
} VM;

enum vm_opcode {VM_NOP,       /* 0   */
                VM_LIT,       /* 1   */
                VM_DUP,       /* 2   */
                VM_DROP,      /* 3   */
                VM_SWAP,      /* 4   */
                VM_PUSH,      /* 5   */
                VM_POP,       /* 6   */
                VM_LOOP,      /* 7   */
                VM_JUMP,      /* 8   */
                VM_RETURN,    /* 9   */
                VM_GT_JUMP,   /* 10  */
                VM_LT_JUMP,   /* 11  */
                VM_NE_JUMP,   /* 12  */
                VM_EQ_JUMP,   /* 13  */
                VM_FETCH,     /* 14  */
                VM_STORE,     /* 15  */
                VM_ADD,       /* 16  */
                VM_SUB,       /* 17  */
                VM_MUL,       /* 18  */
                VM_DIVMOD,    /* 19  */
                VM_AND,       /* 20  */
                VM_OR,        /* 21  */
                VM_XOR,       /* 22  */
                VM_SHL,       /* 23  */
                VM_SHR,       /* 24  */
                VM_ZERO_EXIT, /* 25  */
                VM_INC,       /* 26  */
                VM_DEC,       /* 27  */
                VM_IN,        /* 28  */
                VM_OUT,       /* 29  */
                VM_WAIT       /* 30  */ };

/* interpreter */

#define NEXT switch(vm->image[vm->ip])    \
{                                         \
  case VM_NOP:     goto fVM_NOP;          \
  case VM_LIT:     goto fVM_LIT;          \
  case VM_DUP:     goto fVM_DUP;          \
  case VM_DROP:    goto fVM_DROP;         \
  case VM_SWAP:    goto fVM_SWAP;         \
  case VM_PUSH:    goto fVM_PUSH;         \
  case VM_POP:     goto fVM_POP;          \
  case VM_LOOP:    goto fVM_LOOP;         \
  case VM_JUMP:    goto fVM_JUMP;         \
  case VM_RETURN:  goto fVM_RETURN;       \
  case VM_GT_JUMP: goto fVM_GT_JUMP;      \
  case VM_LT_JUMP: goto fVM_LT_JUMP;      \
  case VM_NE_JUMP: goto fVM_NE_JUMP;      \
  case VM_EQ_JUMP: goto fVM_EQ_JUMP;      \
  case VM_FETCH:   goto fVM_FETCH;        \
  case VM_STORE:   goto fVM_STORE;        \
  case VM_ADD:     goto fVM_ADD;          \
  case VM_SUB:     goto fVM_SUB;          \
  case VM_MUL:     goto fVM_MUL;          \
  case VM_DIVMOD:  goto fVM_DIVMOD;       \
  case VM_AND:     goto fVM_AND;          \
  case VM_OR:      goto fVM_OR;           \
  case VM_XOR:     goto fVM_XOR;          \
  case VM_SHL:     goto fVM_SHL;          \
  case VM_SHR:     goto fVM_SHR;          \
  case VM_ZERO_EXIT: goto fVM_ZERO_EXIT;  \
  case VM_INC:     goto fVM_INC;          \
  case VM_DEC:     goto fVM_DEC;          \
  case VM_IN:      goto fVM_IN;           \
  case VM_OUT:     goto fVM_OUT;          \
  case VM_WAIT:    goto fVM_WAIT;         \
  default:         goto fVM_DEFAULT;      \
}
#define NUM_OPS VM_WAIT
#endif
