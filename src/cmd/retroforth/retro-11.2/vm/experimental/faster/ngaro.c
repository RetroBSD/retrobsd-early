/******************************************************
 * Copyright (c) 2008 - 2011, Charles Childers
 * Copyright (c) 2009 - 2010, Luke Parrish
 * Copyright (c) 2010,        Marc Simpson
 * Copyright (c) 2010,        Matthias Schirm
 * Copyright (c) 2010,        Jay Skeer
 * Copyright (c) 2011,        Kenneth Keating
 ******************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "functions.h"
#include "vm.h"


/******************************************************
 * Main entry point into the VM
 ******************************************************/
int main(int argc, char **argv)
{
  int a, i, trace;

  VM *vm = malloc(sizeof(VM));

  trace = 0;

  strcpy(vm->filename, "retroImage");

  init_vm(vm);
  dev_init(INPUT);

  /* Parse the command line arguments */
  for (i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "--trace") == 0)
    {
      trace = 1;
    }
    else if (strcmp(argv[i], "--with") == 0)
    {
      i++; dev_include(argv[i]);
    }
    else if (strcmp(argv[i], "--help") == 0)
    {
      fprintf(stderr, "%s [options] [imagename]\n", argv[0]);
      fprintf(stderr, "Valid options are:\n");
      fprintf(stderr, "   --about        Display some information about Ngaro\n");
      fprintf(stderr, "   --trace        Execution trace\n");
      fprintf(stderr, "   --with [file]  Treat [file] as an input source\n");
      exit(0);
    }
    else if (strcmp(argv[i], "--about") == 0)
    {
      fprintf(stderr, "Retro Language  [VM: C, console]\n");
      fprintf(stderr, "The Retro language and documentation have been gifted to the\n");
      fprintf(stderr, "public domain. Use it as you see fit.\n\n");
      exit(0);
    }
    else
    {
      strcpy(vm->filename, argv[i]);
    }
  }

  dev_init(OUTPUT);

  a = vm_load_image(vm, vm->filename);
  if (a != -1)
  {
    /* Process the image */
    if (trace == 0)
    {
      for (vm->ip = 0; vm->ip < IMAGE_SIZE; vm->ip++)
      {
        vm_process(vm);
      }
    }
    else
    {
      for (vm->ip = 0; vm->ip < IMAGE_SIZE; vm->ip++)
      {
        display_instruction(vm);
        vm_process(vm);
      }
    }
  }
  /* Once done, cleanup */
  dev_cleanup();
  return 0;
}
