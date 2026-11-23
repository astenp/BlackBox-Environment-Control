#include <stdint.h>

int tsl25721_write(uint8_t r, uint8_t x);
int tsl25721_read(uint8_t r, uint8_t *b, uint16_t l);

void tsl25721_init();

float tsl25721_readLux();