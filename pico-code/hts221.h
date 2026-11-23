#include <stdint.h>

int hts221_write(uint8_t r, uint8_t x);
int hts221_read(uint8_t r, uint8_t *b, uint16_t l);

void hts221_wakeup();

void hts221_humidityCalib(float *H0_rH_x2, float *H1_rH_x2, int16_t *H0_T0_OUT, int16_t *H1_T0_OUT);
void hts221_tempCalib(float *T0_degC, float *T1_degC, int16_t *T0_OUT, int16_t *T1_OUT);

float hts221_readTemp(float T0_degC, float T1_degC, int16_t T0_OUT, int16_t T1_OUT);
float hts221_readHumidity(float H0_rH_x2, float H1_rH_x2, int16_t H0_T0_OUT, int16_t H1_T0_OUT);