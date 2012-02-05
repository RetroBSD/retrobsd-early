/******************************************************
 * Copyright (c) 2008 - 2011, Charles Childers
 * Copyright (c) 2009 - 2010, Luke Parrish
 * Copyright (c) 2010,        Marc Simpson
 * Copyright (c) 2010,        Matthias Schirm
 * Copyright (c) 2010,        Jay Skeer
 * Copyright (c) 2011,        Kenneth Keating
 ******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "functions.h"
#include "vm.h"

#ifndef CURSES
#include <termios.h>

struct termios new_termios, old_termios;
FILE *input[12];
int isp=0;

void rxGetString(VM *vm, int starting)
{
  int i = 0;
  while(vm->image[starting] && i < 1024)
    vm->request[i++] = (char)vm->image[starting++];
  vm->request[i] = 0;
}


/******************************************************
 * Display a character.
 *
 * Will clear the display if a negative value is passed
 ******************************************************/
void dev_putch(int c)
{
  if (c > 0)
  {
    putchar((char)c);
  }
  else
  {
    printf("\033[2J\033[1;1H");
  }
  if (c == 8)
  {
    putchar(32); putchar(8);
  }
}


/******************************************************
 * Update the display
 ******************************************************/
void dev_refresh()
{
}



/******************************************************
 * Get input from an input source
 ******************************************************/
int dev_getch()
{
  int c;

  if ((c = getc(input[isp])) == EOF && input[isp] != stdin)
  {
    fclose(input[isp]);
    isp--;
    return 0;
  }
  if (c == EOF && input[isp] == stdin)
  {
    exit(0);
  }

  return c;
}


/******************************************************
 * Add a file to the input stack
 ******************************************************/
void dev_include(char *s)
{
  FILE *file;

  file = fopen(s, "r");

  if (file)
  {
    isp++;
    input[isp] = file;
  }
}


/******************************************************
 * Initialize real hardware devices
 ******************************************************/
void dev_init(int level)
{
  if (level == 2)
  {
    tcgetattr(0, &old_termios);
    new_termios = old_termios;
    new_termios.c_iflag &= ~(BRKINT+ISTRIP+IXON+IXOFF);
    new_termios.c_iflag |= (IGNBRK+IGNPAR);
    new_termios.c_lflag &= ~(ICANON+ISIG+IEXTEN+ECHO);
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_termios);
  }
  if (level == 1)
  {
    isp = 0;
    input[isp] = stdin;
  }
}


/******************************************************
 * Restore real hardware device settings
 ******************************************************/
void dev_cleanup()
{
  tcsetattr(0, TCSANOW, &old_termios);
}
#else
#include <curses.h>

FILE *input[64];
int isp=0;


/******************************************************
 * Display a character.
 *
 * Will clear the display if a negative value is passed
 ******************************************************/
void dev_putch(int c)
{
  if (c >= 0)
  {
    addch((char)c);
  }
  else
  {
    clear();
  }
  if (c == 8)
  {
    putchar(32); putchar(8);
  }
}


/******************************************************
 * Update the display
 ******************************************************/
void dev_refresh()
{
  refresh();
}


/******************************************************
 * Read a character from an input source
 ******************************************************/
int dev_getch()
{
  int c;

  if ((c = getc(input[isp])) == EOF && input[isp] != stdin)
  {
    fclose(input[isp]);
    isp--;
    return 0;
  }

  if (c == EOF && input[isp] == stdin)
  {
    exit(0);
  }

  if (input[isp] != stdin)
  {
    if (c == 10 || c == 13 || c == 9)
      c = 32;
  }

  if (c == 10)
    c = 0;

  return c;
}


/******************************************************
 * Add a file to the input source list
 ******************************************************/
void dev_include(char *s)
{
  FILE *file;

  file = fopen(s, "r");

  if (file)
  {
    isp++;
    input[isp] = file;
  }
}


/******************************************************
 * Prepare real I/O hardware for the emulator
 ******************************************************/
void dev_init(int level)
{
  if (level == OUTPUT)
  {
    initscr();                /* initialize the curses library */
    cbreak();                 /* take input chars one at a time, no wait for \n */
    scrollok(stdscr, TRUE);   /* Allow the display to scroll */
  }
  if (level == INPUT)
  {
    isp = 0;
    input[isp] = stdin;
  }
}


/******************************************************
 * Cleanup real I/O hardware settings
 ******************************************************/
void dev_cleanup()
{
  endwin();
}
#endif



int rxGetFileHandle(VM *vm)
{
  int i;
  for(i = 1; i < 12; i++)
    if (vm->files[i] == 0)
      return i;
  return 0;
}

int rxOpenFile(VM *vm) {
  int slot, mode, name;
  slot = rxGetFileHandle(vm);
  mode = vm->acc; DROP;
  name = vm->acc; DROP;
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

int rxReadFile(VM *vm) {
  int c = fgetc(vm->files[vm->acc]); DROP;
  return (c == EOF) ? 0 : c;
}

int rxWriteFile(VM *vm) {
  int slot, c, r;
  slot = vm->acc; DROP;
  c = vm->acc; DROP;
  r = fputc(c, vm->files[slot]);
  return (r == EOF) ? 0 : 1;
}

int rxCloseFile(VM *vm) {
  fclose(vm->files[vm->acc]);
  vm->files[vm->acc] = 0;
  DROP;
  return 0;
}

int rxGetFilePosition(VM *vm) {
  int slot = vm->acc; DROP;
  return (int) ftell(vm->files[slot]);
}

int rxSetFilePosition(VM *vm) {
  int slot, pos, r;
  slot = vm->acc; DROP;
  pos  = vm->acc; DROP;
  r = fseek(vm->files[slot], pos, SEEK_SET);
  return r;
}

int rxGetFileSize(VM *vm) {
  int slot, current, r, size;
  slot = vm->acc; DROP;
  current = ftell(vm->files[slot]);
  r = fseek(vm->files[slot], 0, SEEK_END);
  size = ftell(vm->files[slot]);
  fseek(vm->files[slot], current, SEEK_SET);
  return (r == 0) ? size : 0;
}

int rxDeleteFile(VM *vm) {
  int name = vm->acc; DROP;
  rxGetString(vm, name);
  return (unlink(vm->request) == 0) ? -1 : 0;
}

