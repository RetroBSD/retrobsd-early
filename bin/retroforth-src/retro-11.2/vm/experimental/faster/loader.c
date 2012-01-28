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

#include "functions.h"
#include "vm.h"


/******************************************************
 * This is used to load an image to the vm->image[]
 * buffer
 ******************************************************/
int vm_load_image(VM *vm, char *image)
{
  FILE *fp;
  int x;

  if ((fp = fopen(image, "rb")) == NULL)
  {
    return -1;
  }

  x = fread(&vm->image, sizeof(int), IMAGE_SIZE, fp);
  fclose(fp);

  return x;
}


/******************************************************
 * This is used to save an image from the vm->image[]
 * buffer
 ******************************************************/
int vm_save_image(VM *vm, char *image)
{
  FILE *fp;
  int x;

  if ((fp = fopen(image, "wb")) == NULL)
  {
    fprintf(stderr, "Sorry, but I couldn't open %s\n", image);
    dev_cleanup();
    exit(-1);
  }

  if (vm->shrink == 0)
    x = fwrite(&vm->image, sizeof(int), IMAGE_SIZE, fp);
  else
    x = fwrite(&vm->image, sizeof(int), vm->image[3], fp);
  fclose(fp);

  return x;
}
