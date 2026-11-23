/*
* The HTS221 is a module that allows the measurment of temperature and humidity. 
* The output is raw 16-bit signed ADC counts that needs to be calculated using two point linear intepolation.
* Calibration is needed for both types of measurment (reading factory-measured calibration points) and then it is possible to calculate the values using interpolation
*/


#include "hts221.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdint.h>

#define I2C_PORT i2c0
#define HTS221_ADDR 0x5F

#define HTS221_CTRL_REG1  0x20
#define HTS221_CALIB_0    0x30
#define HTS221_HUM_OUT_L  0x28
#define HTS221_TEMP_OUT_L 0x2A

#define HTS221_WAKEUP_OPT 0x85 // PD=1 BDU=1 ODR=1Hz

int hts221_write(uint8_t r, uint8_t x){
    uint8_t buf[2] = {r, x};
    return i2c_write_blocking(I2C_PORT, HTS221_ADDR, buf, 2, false);
}

int hts221_read(uint8_t r, uint8_t *b, uint16_t l){
    r |= 0x80; // need auto-increment for multiple byte data
    i2c_write_blocking(I2C_PORT, HTS221_ADDR, &r, 1, true); 
    return i2c_read_blocking(I2C_PORT, HTS221_ADDR, b, l, false);
}

void hts221_wakeup(){
    hts221_write(HTS221_CTRL_REG1, HTS221_WAKEUP_OPT); 
}

// HTS221 humidity calibration
void hts221_humidityCalib(float *H0_rH_x2, float *H1_rH_x2, int16_t *H0_T0_OUT, int16_t *H1_T0_OUT){
    // real humidity points
    uint8_t clbr_rH[2];
    hts221_read(HTS221_CALIB_0, clbr_rH, 2);
    *H0_rH_x2 = clbr_rH[0] / 2.0f;
    *H1_rH_x2 = clbr_rH[1] / 2.0f;

    // measured humidity points
    uint8_t H0_T0_buf[2], H1_T0_buf[2];
    hts221_read(0x36,H0_T0_buf,2);
    hts221_read(0x3A,H1_T0_buf,2);
    *H0_T0_OUT = (int16_t)((H0_T0_buf[1]<<8) | H0_T0_buf[0]);
    *H1_T0_OUT = (int16_t)((H1_T0_buf[1]<<8) | H1_T0_buf[0]);
}

//HTS221 temperature calibration
void hts221_tempCalib(float *T0_degC, float *T1_degC, int16_t *T0_OUT, int16_t *T1_OUT){
    // real temperature points
    // weird 10-bit format 
    uint8_t T0_degC_x8, T1_degC_x8, T0_T1_msb;
    hts221_read(0x32, &T0_degC_x8, 1);
    hts221_read(0x33, &T1_degC_x8, 1);
    hts221_read(0x35, &T0_T1_msb, 1);

    uint16_t T0_degC_temp = ((T0_T1_msb & 0x03) << 8) | T0_degC_x8;
    uint16_t T1_degC_temp = ((T0_T1_msb & 0x0C) << 6) | T1_degC_x8;

    *T0_degC = T0_degC_temp / 8.0f;
    *T1_degC = T1_degC_temp / 8.0f;

    // measured temperature points
    uint8_t T0_OUT_buf[2], T1_OUT_buf[2];
    hts221_read(0x3C, T0_OUT_buf, 2);
    hts221_read(0x3E, T1_OUT_buf, 2);
    *T0_OUT = (int16_t)((T0_OUT_buf[1] << 8) | T0_OUT_buf[0]);
    *T1_OUT = (int16_t)((T1_OUT_buf[1] << 8) | T1_OUT_buf[0]);
}

// need calibration before running temp or humidity readings
float hts221_readTemp(float T0_degC, float T1_degC, int16_t T0_OUT, int16_t T1_OUT){
    uint8_t t_data[2] = {0, 0};

    hts221_read(HTS221_TEMP_OUT_L, t_data, 2); 

    int T_read = (int16_t)((t_data[1] << 8) | t_data[0]);
    // linear interpolation for sensor read value
    float temperature = T0_degC + (T1_degC - T0_degC) * (float)(T_read - T0_OUT) / (float)(T1_OUT - T0_OUT);
    return temperature;
}

float hts221_readHumidity(float H0_rH_x2, float H1_rH_x2, int16_t H0_T0_OUT, int16_t H1_T0_OUT){
    uint8_t h_data[2] = {0, 0};

    hts221_read(HTS221_HUM_OUT_L, h_data, 2); 

    int H_read = (int16_t)((h_data[1] << 8) | h_data[0]);
    // linear interpolation for sensor read value
    float humidity = H0_rH_x2 + (H1_rH_x2 - H0_rH_x2) * (float)(H_read - H0_T0_OUT) / (float)(H1_T0_OUT - H0_T0_OUT);
    return humidity;
}
