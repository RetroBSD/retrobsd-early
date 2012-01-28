/******************************************************
 * Ngaro
 * Copyright (C) 2008 - 2011, Charles Childers
 ******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "functions.h"
#include "vm.h"
#include "SDL_ColorDef.h"

#include "SDL.h"

/* From vm.c */
extern VM_STATE vm;

FILE *files[12];

int file_free()
{
  int i;
  for(i = 1; i < 12; i++)
  {
    if (files[i] == 0)
      return i;
  }
  return 0;
}

int file_handle() {
  int slot = file_free();
  int mode = TOS; DROP;
  int i, address = TOS; DROP;
  char filename[1024];
  for (i = 0; i < 1024; i++) {
    filename[i] = vm.image[address+i];
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

int file_readc() {
  int c = fgetc(files[TOS]); DROP;
  if ( c == EOF )
    return 0;
  else
    return c;
}

int file_writec() {
  int slot = TOS; DROP;
  int c = TOS; DROP;
  int r = fputc(c, files[slot]);
  if ( r == EOF )
    return 0;
  else
    return 1;
}

int file_closehandle() {
  fclose(files[TOS]);
  files[TOS] = 0;
  DROP;
  return 0;
}

int file_getpos() {
  int slot = TOS; DROP;
  return (int) ftell(files[slot]);
}

int file_seek() {
  int slot = TOS; DROP;
  int pos  = TOS; DROP;
  int r = fseek(files[slot], pos, SEEK_SET);
  return r;
}

int file_size() {
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

int file_delete() {
  int i, address;
  char filename[1024];
  address = TOS; DROP;
  for (i = 0; i < 1024; i++) {
    filename[i] = vm.image[address+i];
    if (! filename[i]) break;
  }
  if (unlink(filename) == 0)
    return -1;
  else
    return 0;
}

typedef struct {
  SDL_Surface *screen;
  SDL_Surface *font;
} DEVICES;

DEVICES io;
int tx, ty, mousex, mousey, mouseb;
int color, gx, gy, gh, gw, scratch;

int black, darkblue, darkgreen, darkcyan, darkred;
int purple, brown, darkgray, gray, blue, green;
int cyan, red, magenta, yellow, white;

void video_color(int c)
{
  switch (c)
  {
    case 0:
      color = black;
      break;
    case 1:
      color = darkblue;
      break;
    case 2:
      color = darkgreen;
      break;
    case 3:
      color = darkcyan;
      break;
    case 4:
      color = darkred;
      break;
    case 5:
      color = purple;
      break;
    case 6:
      color = brown;
      break;
    case 7:
      color = darkgray;
      break;
    case 8:
      color = gray;
      break;
    case 9:
      color = blue;
      break;
    case 10:
      color = green;
      break;
    case 11:
      color = cyan;
      break;
    case 12:
      color = red;
      break;
    case 13:
      color = magenta;
      break;
    case 14:
      color = yellow;
      break;
    case 15:
      color = white;
      break;
    default:
      color = black;
  }
}

void video_pixel(int x, int y)
{
  vm.image[VIDEO_BASE+(x + y * VIDEO_WIDTH)] = color;
}


void line(int x1, int y1, int x2, int y2)
{
  float error, m;
  int x, y;
  x = x1;
  y = y1;
  if (x1 == x2)
  {
    while (y != y2)
    {
      if (y2 - y1 > 0)
         y++;
      else
         y--;
      video_pixel(x,y);
    }
  }
  else
  {
    m = (float)(y2 - y1) / (x2 - x1);
    error = 0;
    video_pixel(x, y);
    while (x != x2)
    {
      error += m;
      if (error > .5)
      {
        if (x2 - x1 > 0)
          y++;
        else
          y--;
        --error;
      }
      if (x2 - x1 > 0)
        x++;
      else
        x--;
      video_pixel(x, y);
    }
  }
}


int equ(float x, float y, int r)
{
  int res = 0;
  if (pow(x, 2) + pow(y, 2) - pow(r, 2) == 0)
    res = 0;
  else if (pow(x, 2) + pow(y, 2) - pow(r, 2) < 0)
    res = -1;
  else if (pow(x, 2) + pow(y, 2) - pow(r, 2) > 0)
    res = 1;
  return res;
}

void setOthers(int x, int y, int xCenter, int yCenter)
{
  video_pixel(xCenter+x, yCenter+y);
  video_pixel(xCenter-x, yCenter+y);
  video_pixel(xCenter+x, yCenter-y);
  video_pixel(xCenter-x, yCenter-y);
  video_pixel(xCenter+y, yCenter+x);
  video_pixel(xCenter-y, yCenter+x);
  video_pixel(xCenter+y, yCenter-x);
  video_pixel(xCenter-y, yCenter-x);
}

void video_circle(int xCenter, int yCenter, int radius)
{
  int x = 0, y = radius;
  setOthers(x, y, xCenter, yCenter);
  while (x < y)
  {
    x++;
    if (equ(x, y - (float) 1 / 2, radius) >= 0)
      y--;
    setOthers(x, y, xCenter, yCenter);
  }
}


/******************************************************
 *|F| void draw_character(int x, int y, int character)
 ******************************************************/
void drawcharacter(int x, int y, int character)
{
  int i, j, screenofs, charofs;

  /* The Ngaro font does not have the first 33    */
  /* characters, so if the requested character is */
  /* less than this, ignore the request           */
  if (character <= 32)
    return;

  character -= 33;

  /* Render the character to the framebuffer*/
  for (i = 0; i < io.font->w; i++)
  {
    screenofs = x + (y + i) * io.screen->pitch;
    charofs = (i + character * io.font->w) * io.font->pitch;
    for (j = 0; j < io.font->w; j++)
    {
      vm.image[screenofs+VIDEO_BASE] = (int)((char *)io.font->pixels)[charofs];
      screenofs++;
      charofs++;
    }
  }
}



/******************************************************
 *|F| int handle_devices(void *unused)
 ******************************************************/
int handle_devices(void *unused)
{
  SDL_Event event;
  int c;

  while (vm.ports[0] == 0)
  {
    if (SDL_PollEvent(&event) != 0)
    {
      switch(event.type)
      {
        case SDL_KEYDOWN:
             vm.ports[1] = (int)event.key.keysym.unicode;
             if (event.key.keysym.sym == SDLK_BACKSPACE)
               vm.ports[1] = 8;
             vm.ports[0] = 1;
             break;
        case SDL_KEYUP:
             break;
        case SDL_MOUSEMOTION:
             mousex = event.motion.x;
             mousey = event.motion.y;
             break;
        case SDL_MOUSEBUTTONDOWN:
             mouseb = 1;
             break;
        case SDL_MOUSEBUTTONUP:
             mouseb = 0;
             break;
        case SDL_QUIT:
             vm.ip = IMAGE_SIZE;
             vm.ports[0] = 1;
             break;
      }
    }
    if (vm.ports[2] == 1)
    {
      c = TOS; DROP
      if (c >= 0)
      {
        drawcharacter(tx, ty, c);
        tx += 9;
        if (c == 8)
        {
          tx -= 16;
          drawcharacter(tx, ty, '_');
        }
        if (c == 10)
        {
          tx  = 0;
          ty += 12;
        }
        if (tx > VIDEO_WIDTH - 9)
        {
          tx  = 0;
          ty += 12;
        }
        if (ty > VIDEO_HEIGHT - 12)
        {
          /* Scroll Up */
          gw = 0;
          for (gx = VIDEO_WIDTH * 12; gx < (VIDEO_WIDTH * VIDEO_HEIGHT) - 12; gx++)
          {
            vm.image[VIDEO_BASE + gw] = vm.image[VIDEO_BASE + gx];
            gw++;
          }

          ty = VIDEO_HEIGHT - 12;

          /* Clear bottom line */
          scratch = color;
          color = black;
          gw = VIDEO_WIDTH;     gh = 12;
          gy = ty;              gx = 0;
          for (; gw > 0; gw--)
          {
            line(gx, gy, gx + gw, gy);
            line(gx + gw, gy, gx + gw, gy + gh);
            line(gx + gw, gy + gh, gx, gy + gh);
            line(gx, gy + gh, gx, gy);
          }
          color = scratch;
          vm.ports[3] = 0;
        }
      }
      else
        clear_display();
      vm.ports[2] = 0;
      vm.ports[0] = 1;
    }
    if (vm.ports[4] == 1)
    {
      vm_save_image(vm.filename);
      vm.ports[4] = 0;
      vm.ports[0] = 1;
    }
    if (vm.ports[4] == 2)
    {
      vm.ports[4] = 0;
      vm.ports[0] = 1;
    }

    /* Create file handle */
    if (vm.ports[4] == -1) {
      vm.ports[4] = file_handle();
      vm.ports[0] = 1;
    }
    
    /* Read a char; response indicates success/failure */
    if (vm.ports[4] == -2) {
      vm.ports[4] = file_readc();
      vm.ports[0] = 1;
    }

    /* Write a char; response indicates success/failure */
    if (vm.ports[4] == -3) {
      vm.ports[4] = file_writec();
      vm.ports[0] = 1;
    }

    /* Close handle */
    if (vm.ports[4] == -4) {
      vm.ports[4] = file_closehandle();
      vm.ports[0] = 1;
    }

    /* Getpos */
    if (vm.ports[4] == -5) {
      vm.ports[4] = file_getpos();
      vm.ports[0] = 1;
    }

    /* Seek */
    if (vm.ports[4] == -6) {
      vm.ports[4] = file_seek();
      vm.ports[0] = 1;
    }

    /* File size */
    if (vm.ports[4] == -7) {
      vm.ports[4] = file_size();
      vm.ports[0] = 1;
    }

    /* File delete */
    if (vm.ports[4] == -8) {
      vm.ports[4] = file_delete();
      vm.ports[0] = 1;
    }

    /* Capabilities */
    if (vm.ports[5] == -1)
    {
      vm.ports[5] = IMAGE_SIZE;
      vm.ports[0] = 1;
    }
    if (vm.ports[5] == -2)
    {
      vm.ports[5] = -1;
      vm.ports[0] = 1;
    }
    if (vm.ports[5] == -3)
    {
      vm.ports[5] = VIDEO_WIDTH;
      vm.ports[0] = 1;
    }
    if (vm.ports[5] == -4)
    {
      vm.ports[5] = VIDEO_HEIGHT;
      vm.ports[0] = 1;
    }
    if (vm.ports[5] == -5)
    {
      vm.ports[5] = vm.sp;
      vm.ports[0] = 1;
    }
    if (vm.ports[5] == -6)
    {
      vm.ports[5] = vm.rsp;
      vm.ports[0] = 1;
    }
    if (vm.ports[5] == -7)
    {
      vm.ports[5] = -1;
      vm.ports[0] = 1;
    }
    if (vm.ports[5] == -8)
    {
      vm.ports[5] = time(NULL);
      vm.ports[0] = 1;
    }
    if (vm.ports[5] == -9)
    {
      vm.ports[5] = 0;
      vm.ports[0] = 1;
      vm.ip = IMAGE_SIZE;
    }

    if (vm.ports[6] == 1)
    {
      video_color(TOS); DROP;
      vm.ports[6] = 0;
      vm.ports[0] = 1;
    }
    if (vm.ports[6] == 2)
    {
      gy = TOS; DROP;
      gx = TOS; DROP;
      video_pixel(gx, gy);
      vm.ports[6] = 0;
      vm.ports[0] = 1;
      vm.ports[3] = 0;
    }
    if (vm.ports[6] == 3)
    {
      gw = TOS; DROP;
      gh = TOS; DROP;
      gy = TOS; DROP;
      gx = TOS; DROP;
      line(gx, gy, gx + gw, gy);
      line(gx + gw, gy, gx + gw, gy + gh);
      line(gx + gw, gy + gh, gx, gy + gh);
      line(gx, gy + gh, gx, gy);
      vm.ports[6] = 0;
      vm.ports[0] = 1;
      vm.ports[3] = 0;
    }
    if (vm.ports[6] == 4)
    {
      gw = TOS; DROP;
      gh = TOS; DROP;
      gy = TOS; DROP;
      gx = TOS; DROP;
      for (; gw > 0; gw--)
      {
        line(gx, gy, gx + gw, gy);
        line(gx + gw, gy, gx + gw, gy + gh);
        line(gx + gw, gy + gh, gx, gy + gh);
        line(gx, gy + gh, gx, gy);
      }
      vm.ports[6] = 0;
      vm.ports[0] = 1;
      vm.ports[3] = 0;
    }
    if (vm.ports[6] == 5)
    {
      gh = TOS; DROP;
      gy = TOS; DROP;
      gx = TOS; DROP;
      line(gx, gy, gx, gy + gh);
      vm.ports[6] = 0;
      vm.ports[0] = 1;
      vm.ports[3] = 0;
    }
    if (vm.ports[6] == 6)
    {
      gw = TOS; DROP;
      gy = TOS; DROP;
      gx = TOS; DROP;
      line(gx, gy, gx + gw, gy);
      vm.ports[6] = 0;
      vm.ports[0] = 1;
      vm.ports[3] = 0;
    }
    if (vm.ports[6] == 7)
    {
      gw = TOS; DROP;
      gy = TOS; DROP;
      gx = TOS; DROP;
      video_circle(gx, gy, gw);
      vm.ports[6] = 0;
      vm.ports[0] = 1;
      vm.ports[3] = 0;
    }
    if (vm.ports[6] == 8)
    {
      gw = TOS; DROP;
      gy = TOS; DROP;
      gx = TOS; DROP;
      for (; gw > 0; gw--)
        video_circle(gx, gy, gw);
      vm.ports[6] = 0;
      vm.ports[0] = 1;
      vm.ports[3] = 0;
    }


    if (vm.ports[7] == 1)
    {
      vm.sp++; vm.data[vm.sp] = mousex;
      vm.sp++; vm.data[vm.sp] = mousey;
      vm.ports[7] = 0;
      vm.ports[0] = 1;
    }
    if (vm.ports[7] == 2)
    {
      vm.sp++; vm.data[vm.sp] = mouseb;
      vm.ports[7] = 0;
      vm.ports[0] = 1;
    }
  }
  return 0;
}



/******************************************************
 *|F| void init_devices()
 ******************************************************/
void init_devices()
{
  SDL_Surface *temp;

  tx = 0;
  ty = 0;

  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
    exit(1);
  }

  /* This causes each key value returned to be unique. */
  SDL_EnableUNICODE(1);

  /* We use an 8-bit display mode. */
  io.screen = SDL_SetVideoMode(VIDEO_WIDTH, VIDEO_HEIGHT, 8, SDL_SWSURFACE);
  SDL_WM_SetCaption("Ngaro", "Ngaro");

  /* Try to load the font. We look in the current directory and in */
  /* /usr/share/ngaro. If we can't find it, report an error and exit. */
  temp = SDL_LoadBMP("font.bmp");
  if (temp == 0)
    temp = SDL_LoadBMP("/usr/share/ngaro/font.bmp");
  if (temp == 0)
  {
    fprintf(stderr, "FATAL: Unable to locate font.bmp!\n");
    exit(1);
  }

  io.font = SDL_ConvertSurface(temp, io.screen->format, SDL_SWSURFACE);
  SDL_FreeSurface(temp);

  /* Map Colors */
  black     = RGB_black(io.screen);
  darkblue  = RGB_darkblue(io.screen);
  darkgreen = RGB_darkgreen(io.screen);
  darkcyan  = RGB_darkcyan(io.screen);
  darkred   = RGB_darkred(io.screen);
  purple    = RGB_purple(io.screen);
  brown     = RGB_brown(io.screen);
  darkgray  = RGB_darkgray(io.screen);
  gray      = RGB_gray(io.screen);
  blue      = RGB_blue(io.screen);
  green     = RGB_green(io.screen);
  cyan      = RGB_cyan(io.screen);
  red       = RGB_red(io.screen);
  magenta   = RGB_magenta(io.screen);
  yellow    = RGB_yellow(io.screen);
  white     = RGB_white(io.screen);
}



/******************************************************
 *|F| void cleanup_devices()
 ******************************************************/
void cleanup_devices()
{
}


/******************************************************
 *|F| void drawpixel(int x, int color)
 ******************************************************/
void drawpixel(int x, int color)
{
  unsigned char *ptr = (unsigned char*)io.screen->pixels;
  ptr[x] = (char)color;
}



/******************************************************
 *|F| int update_display(void *unused)
 ******************************************************/
int update_display(void *unused)
{
  int i;
  if (vm.ports[3] == 0)
  {
    for (i = VIDEO_BASE; i < (VIDEO_BASE+(VIDEO_WIDTH*VIDEO_HEIGHT)); i++)
       drawpixel(i-VIDEO_BASE, vm.image[i]);
    SDL_Flip(io.screen);
    vm.ports[3] = 1;
  }
  return 0;
}


/******************************************************
 *|F| void clear_display()
 ******************************************************/
void clear_display()
{
  int i;

  for (i = VIDEO_BASE; i < (VIDEO_BASE+(VIDEO_WIDTH*VIDEO_HEIGHT)); i++)
    vm.image[i] = 0;

  tx = 0; ty = 0;

  vm.ports[3] = 0;
}
