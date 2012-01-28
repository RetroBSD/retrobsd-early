#ifndef EEPROM_ATMEL_H
#define EEPROM_ATMEL_H
#ifdef EEPROM_ACTIVATED
#error "Only one EEPROM can be activated."
#endif
#define EEPROM_ACTIVATED

#include <avr/eeprom.h>

static CELL load_from_eeprom() {
    CELL ret, key, value;
    uint16_t *a;
    ret = eeprom_read_word(0);
    if (ret <= 0) goto read_error;
    for (int i = 0; i < CHANGE_TABLE_SIZE; ++i)
        change_table[i].full = 0;
    for (int i = 0, k = 1; i < ret; ++i, k += 4) {
        a = (uint16_t*)(k+0); key = eeprom_read_word(a);
        a = (uint16_t*)(k+2); value = eeprom_read_word(a);
        img_put(key, value);
        console_putc('.');
    }
    //for (int i = 0, k = ret * 4 + 1; i < IMAGE_CHANGE_SIZE; ++i, k += 2) {
    //    a = (uint16_t*)k; value = eeprom_read_word(a);
    //    img_put(i + IMAGE_CELLS, value);
    //    console_putc('.');
    //}
    return ret;
read_error:
    console_puts("\nERROR: failed to read from EEPROM. ");
    return 0;
}

static CELL save_to_eeprom() {
    CELL ret = 0, x, y, k, v;
    uint16_t *a;
    eeprom_write_word(0, ret);
    if (eeprom_read_word(0) != 0) goto write_error;
    for (uint16_t i = 0, l = 1; i < CHANGE_TABLE_SIZE; ++i) {
        for (uint16_t j = 0; j < change_table[i].full; ++j, l += 4) {
            k = change_table[i].elements[j].key;
            v = change_table[i].elements[j].value;
            a = (uint16_t*)(l+0); eeprom_write_word(a, k); x = eeprom_read_word(a);
            a = (uint16_t*)(l+2); eeprom_write_word(a, v); y = eeprom_read_word(a);
            if (k != x || v != y) goto write_error;
            ret += 1;
            console_putc('.');
        }
    }
    //for (uint16_t i = 0, l = ret * 4 + 1; i < IMAGE_CHANGE_SIZE; ++i) {
    //    v = image_changes[i];
    //    a = (uint16_t*)l; eeprom_write_word(a, v); y = eeprom_read_word(a);
    //    if (v != y) goto write_error;
    //}
    eeprom_write_word(0, ret);
    x = eeprom_read_word(0);
    if (x != ret) goto write_error;
    return ret;
write_error:
    console_puts("\nERROR: failed to write to EEPROM. ");
    return 0;
}

#endif
