/******************************************************
 * Ngaro
 * Copyright (C) 2008 - 2011, Charles Childers
 ******************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"

#include "functions.h"
#include "vm.h"

extern VM_STATE vm;


/******************************************************
 *|F| int main(int argc, char **argv)
 ******************************************************/
int main(int argc, char **argv)
{
  int a, i;

  printf("Video @ %i\n", VIDEO_BASE);

  strcpy(vm.filename, "retroImage");

  init_vm();
  init_devices();

  vm.shrink = 0;

  /* Parse the command line arguments */
  for (i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "--shrink") == 0)
      vm.shrink = 1;
    else if (strcmp(argv[i], "--help") == 0)
    {
      fprintf(stderr, "%s [options] [imagename]\n", argv[0]);
      fprintf(stderr, "Valid options are:\n");
      fprintf(stderr, "   --shrink   Only save used heap during save operation\n");
      exit(0);
    }
    else
    {
      strcpy(vm.filename, argv[i]);
    }
  }


  /* Load the image */
  a = vm_load_image(vm.filename);

  for (vm.ip = 0; vm.ip < IMAGE_SIZE; vm.ip++)
  {
    vm_process(vm.image[vm.ip]);
    update_display(0);
  }

  /* Once done, cleanup */
  cleanup_devices();
  return 0;
}
