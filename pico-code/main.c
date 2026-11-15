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

// migrate hts221 helper functions to individual file
// hts221 helper functions for reading and writing to regs
uint8_t hts221_write(uint8_t r, uint8_t x){
    uint8_t buf[] = {r, x},n;
    n = i2c_write_blocking(I2C_PORT, HTS221_ADDR, buf, 2, false);
    return n;
}

uint8_t hts221_read(uint8_t r, uint8_t *b, uint16_t l){
    uint8_t n;
    r |= 0x80; // need auto-increment for multiple byte data
    i2c_write_blocking(I2C_PORT, HTS221_ADDR, &r, 1, true); 
    n = i2c_read_blocking(I2C_PORT, HTS221_ADDR, b, l, false);
    return n;
}

int main()
{
    stdio_init_all();


    // I2C Initialisation 400Khz
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    //nucleo has built in pull-up resistors
    //gpio_pull_up(I2C_SDA);
    //gpio_pull_up(I2C_SCL);

    // UART setup
    uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    // gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    // gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // Use some the various UART functions to send out data
    // In a default system, printf will also output via the default UART
    
    // Send out a string, with CR/LF conversions
    //uart_puts(UART_ID, " Hello, UART!\n");

    hts221_write(HTS221_CTRL_REG1, 0x85); // wake up hts221 PD=1 BDU=1 ODR=1Hz

    // HTS221 humidity calibration
    // real humidity points
    uint8_t clbr_rH[2];
    hts221_read(HTS221_CALIB_0, clbr_rH, 2);
    float H0_rH_x2 = clbr_rH[0] / 2.0f;
    float H1_rH_x2 = clbr_rH[1] / 2.0f;

    // measured humidity points
    uint8_t H0_T0_buf[2], H1_T0_buf[2];
    hts221_read(0x36,H0_T0_buf,2);
    hts221_read(0x3A,H1_T0_buf,2);
    int16_t H0_T0_OUT = (int16_t)((H0_T0_buf[1]<<8) | H0_T0_buf[0]);
    int16_t H1_T0_OUT = (int16_t)((H1_T0_buf[1]<<8) | H1_T0_buf[0]);

    //HTS221 temperature calibration
    // real temperature points
    // weird 10-bit format 
    uint8_t T0_degC_x8, T1_degC_x8, T0_T1_msb;
    hts221_read(0x32, &T0_degC_x8, 1);
    hts221_read(0x33, &T1_degC_x8, 1);
    hts221_read(0x35, &T0_T1_msb, 1);

    uint16_t T0_degC_temp = ((T0_T1_msb & 0x03) << 8) | T0_degC_x8;
    uint16_t T1_degC_temp = ((T0_T1_msb & 0x0C) << 6) | T1_degC_x8;

    float T0_degC = T0_degC_temp / 8.0f;
    float T1_degC = T1_degC_temp / 8.0f;

    // measured temperature points
    uint8_t T0_OUT_buf[2], T1_OUT_buf[2];
    hts221_read(0x3C, T0_OUT_buf, 2);
    hts221_read(0x3E, T1_OUT_buf, 2);
    int16_t T0_OUT = (int16_t)((T0_OUT_buf[1] << 8) | T0_OUT_buf[0]);
    int16_t T1_OUT = (int16_t)((T1_OUT_buf[1] << 8) | T1_OUT_buf[0]);


    while (true) {

        // HTS221 data reading and conversion to real value
        uint8_t h_data[2] = {0, 0};
        uint8_t t_data[2] = {0, 0};
        uint8_t n,m;

        n = hts221_read(HTS221_HUM_OUT_L, h_data, 2); 
        //printf("%d\n",n);
        m = hts221_read(HTS221_TEMP_OUT_L, t_data, 2); 
        //printf("%d\n",m);

        //printf("h_r : %d  t_r: %d \n", n,m);

        int H_read = (int16_t)((h_data[1] << 8) | h_data[0]);
        int T_read = (int16_t)((t_data[1] << 8) | t_data[0]);

        // linear interpolation calc for value from sensor
        float humidity = H0_rH_x2 + (H1_rH_x2 - H0_rH_x2) * (float)(H_read - H0_T0_OUT) / (float)(H1_T0_OUT - H0_T0_OUT);
        float temperature = T0_degC + (T1_degC - T0_degC) * (float)(T_read - T0_OUT) / (float)(T1_OUT - T0_OUT);

        printf("h: %f  t: %f \n", humidity, temperature);

        sleep_ms(1000);
        }
    return 0;
}
