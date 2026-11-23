/*
* The TSL25721 is a light-to-digital ambient light sensor with programmable analog gain and high dynamic range
* Before turning the sensor on we define its configuration. The value defines should be aligned with the enviorment, most notably the programmable gain
* To convert raw measurements from the two-channel photodiode architecture to human-readable data, we use an empirical subtraction formula defined in the sensor specifications
*/

#include "tsl25721.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdint.h>

#define I2C_PORT i2c0
#define TSL25721_ADDR 0x39

// TSL25721 reg map defines
#define TSL25721_CMD_SINGLE 0x80 // single byte option for read/write
#define TSL25721_CMD_AUTOINC 0xA0 // auto increment for multiple bytes
#define TSL25721_ENABLE 0x00
#define TSL25721_REG_ATIME 0x01
#define TSL25721_CONFIG 0x0D
#define TSL25721_CONTROL 0x0F
#define TSL25721_C0DATAL 0x14

// TSL25721 value defines
#define TSL25721_VAL_ATIME 0xDB // set to 219 calculates into around 100ms, which helps with potential flickering problems with 50Hz light sources
#define TSL25721_GAIN 0x00  // programmable analog gain -> 0x00=1x, 0x01=8x, 0x02=16x, 0x03=120x 
#define TSL25721_GAIN_VAL 1.0f // this needs to be set to the correct value corresponding to TSL25721_GAIN
#define TSL25721_GLASS_ATTENUATION 1.0f



int tsl25721_write(uint8_t r, uint8_t x){
    uint8_t buf[2] = {(uint8_t)(TSL25721_CMD_SINGLE | r), x};
    return i2c_write_blocking(I2C_PORT, TSL25721_ADDR, buf, 2, false);
}

int tsl25721_read(uint8_t r, uint8_t *b, uint16_t l){
    r |= 0xA0;
    i2c_write_blocking(I2C_PORT, TSL25721_ADDR, &r, 1, true);
    return i2c_read_blocking(I2C_PORT, TSL25721_ADDR, b, l, false);
}

void tsl25721_init(){
    // TSL25721 init
    // disable sensor for configuration
    tsl25721_write(TSL25721_ENABLE, 0x00);

    tsl25721_write(TSL25721_REG_ATIME, TSL25721_VAL_ATIME); // integration time -> tint = (256 - ATIME) * 2.73ms
    tsl25721_write(TSL25721_CONFIG, 0x00); // additional config needs to be 0 for 120x AGAIN if needed
    tsl25721_write(TSL25721_CONTROL, TSL25721_GAIN);

    // enable sensor after configuration
    tsl25721_write(TSL25721_ENABLE, 0x01); // enable generator PON=1 | AEN=0
    sleep_ms(5); // minimal time stabilization time for generator is 2,73ms
    tsl25721_write(TSL25721_ENABLE, 0x03); // enable sensor PON=1 | AEN=1

}

float tsl25721_readLux(){
    uint8_t data[4] = {0,0,0,0};
    tsl25721_read(TSL25721_C0DATAL, data, 4);

    uint16_t raw0 = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    uint16_t raw1 = (uint16_t)data[2] | ((uint16_t)data[3] << 8);

    float atime = (256.0f - (float)TSL25721_VAL_ATIME) * 2.73f; // ms
    float cpl = (atime * TSL25721_GAIN_VAL) / (TSL25721_GLASS_ATTENUATION * 60.0f); 

    float lux1 = ((float)raw0 - 1.87f * (float)raw1)/cpl;
    float lux2 = (0.63f * (float)raw0 - 1.0f * (float)raw1)/cpl;

    float lux = 0.0f;
    if (lux1 > lux) lux = lux1;
    if (lux2 > lux) lux = lux2;

    return lux;
}
