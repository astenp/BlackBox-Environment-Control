/**
*    Raspberry Pi Pico sensor board  
* 
*    Date: 23.11.2026
*    Authors: Bartlomiej Sokal, Przemyslaw Lukaszyk
**/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hts221.h"
#include "stlm75.h"
#include "tsl25721.h"

// I2C defines
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

// i2c slave address defines for sensors
#define STLM75_ADDR   0x48
#define TSL25721_ADDR 0x39
#define LPS331_ADDR   0x5D
#define HTS221_ADDR   0x5F

// LPS331 reg map defines

// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart1
#define BAUD_RATE 115200

// Use pins 4 and 5 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 4
#define UART_RX_PIN 5

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

    hts221_wakeup(); 

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
