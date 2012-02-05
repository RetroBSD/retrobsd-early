/******************************************************
 * Ngaro
 * Copyright (C) 2008 - 2011, Charles Childers
 ******************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include "functions.h"
#include "vm.h"

extern VM_STATE vm;



/******************************************************
 *|F| int vm_load_image(char *image)
 * Description:
 *   This is used to load an image to the vm.image[]
 *   buffer
 ******************************************************/
int vm_load_image(char *image)
{
  FILE *fp;
  int x;

  if ((fp = fopen(image, "rb")) == NULL)
  {
    return -1;
  }

  x = fread(&vm.image, sizeof(int), IMAGE_SIZE, fp);
  fclose(fp);

  return x;
}


/******************************************************
 *|F| int vm_save_image(char *image)
 * Description:
 *   This is used to save an image from the vm.image[]
 *   buffer
 ******************************************************/
int vm_save_image(char *image)
{
  FILE *fp;
  int x;

  if ((fp = fopen(image, "wb")) == NULL)
  {
    fprintf(stderr, "Sorry, but I couldn't open %s\n", image);
    cleanup_devices();
    exit(-1);
  }

  if (vm.shrink == 0)
    x = fwrite(&vm.image, sizeof(int), IMAGE_SIZE, fp);
  else
    x = fwrite(&vm.image, sizeof(int), vm.image[3], fp);
  fclose(fp);

  return x;
}
