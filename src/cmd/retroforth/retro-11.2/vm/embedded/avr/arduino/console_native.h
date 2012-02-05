#ifndef CONSOLE_NATIVE_H
#define CONSOLE_NATIVE_H
#ifdef CONSOLE_ACTIVATED
#error "Only one console can be activated."
#endif
#define CONSOLE_ACTIVATED

static struct { struct termios newt, oldt; } console;

static void console_prepare() {
    tcgetattr(0, &console.oldt);
    console.newt = console.oldt;
    console.newt.c_iflag &= ~(BRKINT+ISTRIP+IXON+IXOFF);
    console.newt.c_iflag |= (IGNBRK+IGNPAR);
    console.newt.c_lflag &= ~(ICANON+ISIG+IEXTEN+ECHO);
    console.newt.c_cc[VMIN] = 1;
    console.newt.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &console.newt);
}

static void console_finish() {
  tcsetattr(0, TCSANOW, &console.oldt);
}

static void console_putc(char c) {
    putchar(c);
}

static char console_getc(void) {
    return getchar();
}

#endif
