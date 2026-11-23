/*
* The STLM75 is a temperature sensor best working as a watchdog.
* Its 0.5 celsius resolution makes the readings be much less precise than the hts221.
* In theory can be used as backup for the hts221 would it fail, since this sensor doesn't depend on wakeup routines and instantly reads when powered.
*/

#include "stlm75.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdint.h>

#define I2C_PORT i2c0
#define STLM75_ADDR   0x48

#define STLM75_TEMP_REG 0x00

float stlm75_temp(){
    // STLM75 read temperature and convert into celsius value
    // since HTS221 is more precise STLM75 should be used as backup
    uint8_t reg = STLM75_TEMP_REG;
    uint8_t data[2] = {0, 0};

    i2c_write_blocking(I2C_PORT, STLM75_ADDR, &reg, 1, true);
    
    i2c_read_blocking(I2C_PORT, STLM75_ADDR, data, 2, false);

    int16_t rawtemp = (data [0]<<8) | data[1];
    rawtemp >>= 7;
    float temp = rawtemp * 0.5f;
    return temp;
}