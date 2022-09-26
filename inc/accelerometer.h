#ifndef __ACCELEROMETER_H__
#define __ACCELEROMETER_H__
#include <stdint.h>

void init_i2c(void);
void i2c_waitidle(void);
void i2c_start(uint32_t devaddr, uint8_t size, uint8_t dir);
void i2c_stop(void);
int i2c_checknack(void);
void i2c_clearnack(void);
int i2c_senddata(uint8_t devaddr, const void *data, uint8_t size);
int i2c_recvdata(uint8_t devaddr, void *data, uint8_t size);
void init_adxl();
int read_tilt();
void nano_wait(int n);

#define ADXL_ADDR 0x1d

#endif
// TO USE ACCELEROMETER:
// 1. call init_i2c at the top of main
// 2. call init_adxl at the top of main
// 3. call read_tilt to get the tilt
//      - return -1 means left
//      - return 0 means neither left nor right
//      - return 1 means right

