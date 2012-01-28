#ifndef CONSOLE_MEGA2560_H
#define CONSOLE_MEGA2560_H
#ifdef CONSOLE_ACTIVATED
#error "Only one console can be activated."
#endif
#define CONSOLE_ACTIVATED

static void console_prepare() {
    UBRR0   = F_CPU/16/BAUD - 1;
    UCSR0B |= (1 << RXEN0);
    UCSR0B |= (1 << TXEN0);
    UCSR0C  = (1 << USBS0) | (1 << UCSZ00) | (1 << UCSZ01);
}

static void console_finish() {
}

static void console_putc(char c) {
    if (c == '\n') {
        while (!(UCSR0A & (1<<UDRE0))); UDR0 = '\r';
        while (!(UCSR0A & (1<<UDRE0))); UDR0 = '\n';
    } else if (c != '\r') while (!(UCSR0A & (1<<UDRE0))); UDR0 = c;
}

static char console_getc(void) {
    char c;
    do {
        while (!(UCSR0A & (1<<RXC0)));
        c = UDR0;
    } while (c == '\r');
    return c;
}

#endif
