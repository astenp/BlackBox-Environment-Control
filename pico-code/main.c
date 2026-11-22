/**
*    Raspberry Pi Pico sensor board  
* 
*    Date: 15.11.2026
*    Authors: Bartlomiej Sokal, Przemyslaw Lukaszyk
**/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"

// I2C defines
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

// i2c slave address defines for sensors
#define STLM75_ADDR   0x48
#define TSL25721_ADDR 0x39
#define LPS331_ADDR   0x5D
#define HTS221_ADDR   0x5F

// STLM75 reg map defines
#define STLM75_TEMP_REG 0x00

// HTS221 reg map defines
#define HTS221_CTRL_REG1  0x20
#define HTS221_CALIB_0    0x30
#define HTS221_HUM_OUT_L  0x28
#define HTS221_TEMP_OUT_L 0x2A

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

// LPS331 reg map defines

// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart1
#define BAUD_RATE 115200

// Use pins 4 and 5 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 4
#define UART_RX_PIN 5


// bus scan code for testing
bool reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void i2c_bus_scan(){
           printf("\nI2C Bus Scan\n");
        printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

         for (int addr = 0; addr < (1 << 7); ++addr) {
        if (addr % 16 == 0) {
            printf("%02x ", addr);
        }

        // Perform a 1-byte dummy read from the probe address. If a slave
        // acknowledges this address, the function returns the number of bytes
        // transferred. If the address byte is ignored, the function returns
        // -1.

        // Skip over any reserved addresses.
        int ret;
        uint8_t rxdata;
        if (reserved_addr(addr))
            ret = PICO_ERROR_GENERIC;
        else
            ret = i2c_read_blocking(i2c_default, addr, &rxdata, 1, false);

        printf(ret < 0 ? "." : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
        }
        printf("Done.\n");

        sleep_ms(3000);
}

// migrate stlm75 read temp to individual file
void STLM75_temp(){
    // STLM75 read temperature and convert into celsius value
    // since HTS221 is more precise STLM75 should be used as backup
    uint8_t reg = STLM75_TEMP_REG;
    uint8_t data[2] = {0, 0};
    uint16_t n, m;

    n = i2c_write_blocking(I2C_PORT, STLM75_ADDR, &reg, 1, true);
        //printf("stlm75 write %d  ", n);
        
    m = i2c_read_blocking(I2C_PORT, STLM75_ADDR, data, 2, false);
        //printf("stlm75 read %d\n", m);

    if(n == 1 && m == 2){
        //printf("%d %d \n", data[0], data[1]);
        
        int16_t rawtemp = (data [0]<<8) | data[1];
        rawtemp >>= 7;
        float temp = rawtemp * 0.5f;
        printf("%f\n", temp);
    }
}

//
// HTS221 FUNCTIONS
//

int hts221_write(uint8_t r, uint8_t x){
    uint8_t buf[2] = {r, x};
    return i2c_write_blocking(I2C_PORT, HTS221_ADDR, buf, 2, false);
}

int hts221_read(uint8_t r, uint8_t *b, uint16_t l){
    r |= 0x80; // need auto-increment for multiple byte data
    i2c_write_blocking(I2C_PORT, HTS221_ADDR, &r, 1, true); 
    return i2c_read_blocking(I2C_PORT, HTS221_ADDR, b, l, false);
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

//
// TSL25721 FUNCTIONS
//

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
    tsl25721_write(TSL25721_CONFIG, 0x00); // additional config needs to be 0 for 120x AGAIN
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

int main()
{
    stdio_init_all();


    // I2C Initialisation 400Khz
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    // nucleo expander has built in pull-up resistors
    //gpio_pull_up(I2C_SDA);
    //gpio_pull_up(I2C_SCL);

    // UART setup
    //uart_init(UART_ID, BAUD_RATE);
    //gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    //gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    //uart_puts(UART_ID, " Hello, UART!\n");

    hts221_write(HTS221_CTRL_REG1, 0x85); // wake up hts221 PD=1 BDU=1 ODR=1Hz

    // HTS221 calibration
    float H0_rH_x2;
    float H1_rH_x2;
    int16_t H0_T0_OUT;
    int16_t H1_T0_OUT;
    hts221_humidityCalib(&H0_rH_x2, &H1_rH_x2, &H0_T0_OUT, &H1_T0_OUT);

    float T0_degC;
    float T1_degC;
    int16_t T0_OUT;
    int16_t T1_OUT;
    hts221_tempCalib(&T0_degC, &T1_degC, &T0_OUT, &T1_OUT);

    tsl25721_init();

    while (true) {

        printf("------------------\n");
        
        float h = hts221_readHumidity(H0_rH_x2,H1_rH_x2,H0_T0_OUT,H1_T0_OUT);
        float t = hts221_readTemp(T0_degC, T1_degC, T0_OUT, T1_OUT);

        printf("HTS221 READ: humidity: %f  temperature: %f \n", h, t);
      
        //sleep_ms(150);
        
        float lux = tsl25721_readLux();
        
        printf("TSL25721 READ: lux: %f\n",lux);

        sleep_ms(1000);
        }
    return 0;
}
