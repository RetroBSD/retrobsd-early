/******************************************************
 * Ngaro
 * Copyright (C) 2008 - 2011, Charles Childers
 ******************************************************/

#include "functions.h"
#include "vm.h"

#include "SDL.h"

/* Variables specific to the VM */
VM_STATE vm;



/******************************************************
 *|F| void init_vm()
 *
 * This zeros out everything in the VM_STATE struct
 * to give us a known starting point.
 ******************************************************/
void init_vm()
{
   int a;
   vm.ip = vm.sp = vm.rsp = 0;
   for (a = 0; a < STACK_DEPTH; a++)
      vm.data[a] = 0;
   for (a = 0; a < ADDRESSES; a++)
      vm.address[a] = 0;
   for (a = 0; a < IMAGE_SIZE; a++)
      vm.image[a] = 0;
   for (a = 0; a < 1024; a++)
      vm.ports[a] = 0;
}



/******************************************************
 *|F| void vm_process()
 *
 * This is a simple switch-based processor with one
 * case for each opcode. It's not the fastest approach,
 * but is easy enough to follow, add on to, and works
 * well enough for my current needs.
 ******************************************************/
void vm_process(int opcode) {
  int a, b;

  switch(opcode)
  {
    case VM_NOP:
         break;
    case VM_LIT:
         vm.sp++;
         vm.ip++;
         TOS = vm.image[vm.ip];
         break;
    case VM_DUP:
         vm.sp++;
         vm.data[vm.sp] = NOS;
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
         vm.rsp++;
         TORS = TOS;
         DROP
         break;
    case VM_POP:
         vm.sp++;
         TOS = TORS;
         vm.rsp--;
         break;
    case VM_LOOP:
         TOS--;
         if (TOS != 0 && TOS > -1)
         {
           vm.ip++;
           vm.ip = vm.image[vm.ip] - 1;
         }
         else
         {
           vm.ip++;
           DROP;
         }
         break;
    case VM_JUMP:
         vm.ip++;
         vm.ip = vm.image[vm.ip] - 1;
         if (vm.ip < 0)
         {
           vm.ip = IMAGE_SIZE;
         }
         else
         {
           if (vm.image[vm.ip+1] == 0)
             vm.ip++;
           if (vm.image[vm.ip+1] == 0)
             vm.ip++;
         }
         break;
    case VM_RETURN:
         vm.ip = TORS;
         vm.rsp--;
         if (vm.image[vm.ip+1] == 0)
           vm.ip++;
         if (vm.image[vm.ip+1] == 0)
           vm.ip++;
         break;
    case VM_GT_JUMP:
         vm.ip++;
         if(NOS > TOS)
           vm.ip = vm.image[vm.ip] - 1;
         DROP DROP
         break;
    case VM_LT_JUMP:
         vm.ip++;
         if(NOS < TOS)
           vm.ip = vm.image[vm.ip] - 1;
         DROP DROP
         break;
    case VM_NE_JUMP:
         vm.ip++;
         if(TOS != NOS)
           vm.ip = vm.image[vm.ip] - 1;
         DROP DROP
         break;
    case VM_EQ_JUMP:
         vm.ip++;
         if(TOS == NOS)
           vm.ip = vm.image[vm.ip] - 1;
         DROP DROP
         break;
    case VM_FETCH:
         TOS = vm.image[TOS];
         break;
    case VM_STORE:
         vm.image[TOS] = NOS;
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
         if (TOS == 0)
         {
           DROP
           vm.ip = TORS;
           vm.rsp--;
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
         TOS = vm.ports[a];
         vm.ports[a] = 0;
         break;
    case VM_OUT:
         vm.ports[0] = 0;
         vm.ports[TOS] = NOS;
         DROP DROP
         break;
    case VM_WAIT:
         handle_devices(0);
         break;
    default:
         vm.rsp++;
         TORS = vm.ip;
         vm.ip = vm.image[vm.ip] - 1;
         if (vm.ip < 0)
         {
           vm.ip = IMAGE_SIZE;
         }
         else
         {
           if (vm.image[vm.ip+1] == 0)
             vm.ip++;
           if (vm.image[vm.ip+1] == 0)
             vm.ip++;
         }
         break;
  }
}
