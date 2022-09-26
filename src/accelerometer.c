#include "accelerometer.h"
#include "stm32f0xx.h"
#include "game.h"

void init_i2c(void) {
    // pb6 -> SCL
    // pb7 -> SDA
    // both i2c1 (AF1)
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER &= ~0x5000;
    GPIOB->MODER |= 0xa000;
    GPIOB->AFR[0] |= 0x11000000; // set p6,7 to AF1

    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    I2C1->CR1 &= ~I2C_CR1_PE; // turn off pe in cr1 before making changes
    I2C1->CR1 &= ~I2C_CR1_ANFOFF; // clears analog noise filter off (so that it's on)
    I2C1->CR1 &= ~I2C_CR1_ERRIE; // disable error interrupt
    I2C1->CR1 &= ~I2C_CR1_NOSTRETCH; // enable clock stretching

    I2C1->TIMINGR = 0; // clear the timing register (ok...?)
    I2C1->TIMINGR &= ~(I2C_TIMINGR_PRESC); // set prescaler to 0
    I2C1->TIMINGR |= 3<<20; // set scldel[3:0] to 3
    I2C1->TIMINGR |= 1<<16; // set sdadel[3:0] to 1
    I2C1->TIMINGR |= 3<<8; // set sclh[7:0] to 3
    I2C1->TIMINGR |= 9<<0; // set sdah[7:0] to 9
    I2C1->OAR1 &= ~I2C_OAR1_OA1EN; // disable "own address"
    I2C1->OAR2 &= ~I2C_OAR2_OA2EN; // disable the other own address (??)
    I2C1->CR2 &= ~I2C_CR2_ADD10; // turns off this bit, which means master operates in 7 bit addressing mode
    I2C1->CR2 |= I2C_CR2_AUTOEND; // enable automatic end
    I2C1->CR1 |= I2C_CR1_PE; // finally, turn on the thing
}

void i2c_waitidle(void) {
    while((I2C1->ISR & I2C_ISR_BUSY) == I2C_ISR_BUSY); // hold while the busy bit is high
}

void i2c_start(uint32_t devaddr, uint8_t size, uint8_t dir) {
    uint32_t tmpreg = I2C1->CR2;
    tmpreg &= ~(I2C_CR2_SADD |
                I2C_CR2_NBYTES |
                I2C_CR2_RELOAD |
                I2C_CR2_AUTOEND |
                I2C_CR2_RD_WRN |
                I2C_CR2_START |
                I2C_CR2_STOP);
    if(dir == 1){
        tmpreg |= I2C_CR2_RD_WRN; // read from slave
    } else {
        tmpreg &= ~I2C_CR2_RD_WRN; // write to slave
    }
    tmpreg |= ((devaddr<<1) & I2C_CR2_SADD) | ((size<<16) & I2C_CR2_NBYTES);
    tmpreg |= I2C_CR2_START;
    I2C1->CR2 = tmpreg;
}

void i2c_stop(void) {
    if(I2C1->ISR & I2C_ISR_STOPF){
        return;
    }
    // generate transfer after current byte has been transfered
    I2C1->CR2 |= I2C_CR2_STOP;
    // wait until stopf flag is reset
    while((I2C1->ISR & I2C_ISR_STOPF) == 0);
    I2C1->ICR |= I2C_ICR_STOPCF; // write to clear stopf flag
}

int i2c_checknack(void) {
    return I2C1->ISR & I2C_ISR_NACKF;
}

void i2c_clearnack(void) {
    I2C1->ICR |= I2C_ICR_NACKCF;
}

int i2c_senddata(uint8_t devaddr, const void *data, uint8_t size) {
    int i;
    if(size <= 0 || data==0){return -1;}
    uint8_t *udata = (uint8_t*)data;
    i2c_waitidle();
    i2c_start(devaddr, size, 0); // last arg is dir = 0, send data to the slave (write)
    for(i = 0; i < size; i++){
        int count = 0;
        while((I2C1->ISR & I2C_ISR_TXIS) == 0){
            count += 1;
            if(count > 1000000){return -1;}
            if(i2c_checknack()){
                i2c_clearnack();
                i2c_stop();
                return -1;
            }
        }
        I2C1->TXDR = udata[i] & I2C_TXDR_TXDATA;
    }
    while((I2C1->ISR & I2C_ISR_TC) == 0 && (I2C1->ISR & I2C_ISR_NACKF) == 0);
    if((I2C1->ISR & I2C_ISR_NACKF) != 0){return -1;}
    i2c_stop();
    return 0;
}

int i2c_recvdata(uint8_t devaddr, void *data, uint8_t size) {
    int i;
    if(size <= 0 || data == 0){return -1;}
    uint8_t *udata = (uint8_t*)data;
    i2c_waitidle();
    i2c_start(devaddr, size, 1);
    for(i = 0; i < size; i++){
        int count = 0;
        while((I2C1->ISR & I2C_ISR_RXNE) == 0){
            count += 1;
            if(count > 1000000) {return -1;}
            if(i2c_checknack()){
                i2c_clearnack();
                i2c_stop();
                return -1;
            }
        }

        udata[i] = I2C1->RXDR;
    }
    while((I2C1->ISR & I2C_ISR_TC) == 0 && (I2C1->ISR & I2C_ISR_NACKF) == 0);
    if((I2C1->ISR & I2C_ISR_NACKF) != 0){return -1;}
    i2c_stop();
    return 0;
}

void init_adxl(){
    // write a 1 to the measure bit of power control register
    uint8_t ADXL_PWRCTLR = 0x2d;
    uint8_t data[2] = {ADXL_PWRCTLR, 0x08};
    i2c_senddata(ADXL_ADDR, data, 2);

    // setup device to read from x axis
    uint8_t temp = 0x32;
    i2c_senddata(ADXL_ADDR, &temp, 1);

}

int read_tilt(){
    uint8_t reading[2] = {0x00, 0x00};
    uint8_t x_axis_addr = 0x32;
    i2c_senddata(ADXL_ADDR, &x_axis_addr, 1);
    i2c_recvdata(ADXL_ADDR, reading, 2);
    int16_t result = reading[1] << 8 | reading[0];

    if(result > 0x30){
        // turning right
        return 2*RIGHT;
    }
    else if(result < -0x30){
        // turning left
        return 2*LEFT;
    }
    else if(result > 0x18){
      // turning right
      return RIGHT;
    }
    else if(result < -0x18){
      // turning left
      return LEFT;
    } else {
        // not turning
        return 0;
    }

}


void nano_wait(int n){
    for(int i = n; i >= n; i-=83);
}
