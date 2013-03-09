#ifndef DEBUG_H
#define DEBUG_H

void debug_init();
void debug_char(uint8_t);
void debug_str(char *);
void debug_hex8(uint8_t);
void debug_hex16(uint16_t);
void debug_dec16(uint16_t);

#endif
