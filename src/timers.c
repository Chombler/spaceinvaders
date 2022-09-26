#include "stm32f0xx.h"
#include <stdint.h>
#include "game.h"
#include "midi.h"
#include "midiplay.h"
#include "lcd.h"
#include "music.h"


void init_lcd_spi(void)
{
    //initialize gpiob
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER |= 0x30C30000;
    GPIOB->MODER &= ~0x20820000;
    GPIOB->ODR |= 0x4900;
    GPIOB->MODER |= 0xCC0;
    GPIOB->MODER &= ~0x440;
    GPIOB->AFR[0] &= ~0xF0F000;
    //initialize spi
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    SPI1->CR1 &= ~1<<6; //turn off spe
    SPI1->CR1 &= ~0x38; //baud rate
    SPI1->CR1 |= 1<<2; //master mode
    SPI1->CR2 = 0x700; //8 bit word size
    SPI1->CR1 |= 1<<8; //ssm
    SPI1->CR1 |= 1<<9; //ssi
    SPI1->CR1 |= 1<<6; //turn on spe
}
void setup_buttons(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    GPIOC->MODER &= ~GPIO_MODER_MODER3;
    GPIOC->PUPDR &= ~GPIO_PUPDR_PUPDR3;
    GPIOC->PUPDR |= GPIO_PUPDR_PUPDR3_1;
}
// - We use the Timer 6 IRQ to recompute samples and feed those
// samples into the DAC by calling TIM6_DAC_IRQHandler at 20 kHz
// - The DAC outputs analog samples on PA4 and is triggered by TIM6 TRGO.
// - Timer 2 invokes the Update interrupt every n microseconds.
// Basically ARR = n-1. Also the ARPE bit is set in the CR1 so that the timer
// waits until the next update before changing the effective ARR value.
// - NVIC_SetPriority() is used to set a low priority for Timer 2 interrupt.
void TIM6_DAC_IRQHandler(void)
{
    // TODO: Remember to acknowledge the interrupt right here.
    int EN;
    EN = 1<<0;
    TIM6->SR &= ~EN;
    int sample = 0;
    for(int x=0; x < sizeof voice / sizeof voice[0]; x++) {
      if (voice[x].in_use) {
          voice[x].offset += voice[x].step;
          if (voice[x].offset >= N<<16)
              voice[x].offset -= N<<16;
          voice[x].last_sample = voice[x].wavetable[voice[x].offset>>16];
          sample += (voice[x].last_sample * voice[x].volume) >> 4;
      }
      else if (voice[x].volume != 0) {
          sample += (voice[x].last_sample * voice[x].volume) >> 4;
          voice[x].volume --;
      }
    }
    sample = (sample >> 10) + 2048;
    if (sample > 4095)
        sample = 4095;
    else if (sample < 0)
        sample = 0;
    DAC->DHR12R1 = sample;
}
void init_tim6(void)
{
    // TODO: you fill this in.
    int EN;
    //enable rcc
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    //set to RATE times per second
    TIM6->ARR = 2400 - 1;
    //set dier
    TIM6->DIER |= TIM_DIER_UIE;
    //enable counter
    TIM6->CR1 |= TIM_CR1_CEN;
    //enable dac
    EN = 0x20;
    TIM6->CR2 |= EN;
    //set nvic
    NVIC->ISER[0] = 1<<TIM6_DAC_IRQn;
    NVIC_SetPriority(TIM6_DAC_IRQn, 0);
}
void init_dac(void)
{
    // TODO: you fill this in.
    int en;
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    en = 0x3d;
    DAC->CR |= en;
    en = 0x38;
    DAC->CR &= ~en;
}
void TIM2_IRQHandler(void)
{
    // TODO: Remember to acknowledge the interrupt right here!
    TIM2->SR &= ~TIM_SR_UIF;
    midi_play();
}
void init_tim2(int n) {
    // TODO: you fill this in.
    //enable rcc
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    //set to 4 times per second
    TIM2->PSC = 48 - 1;
    TIM2->ARR = n - 1;
    //set dier
    TIM2->DIER |= TIM_DIER_UIE;
    //enable counter
    TIM2->CR1 |= TIM_CR1_CEN;
    TIM2->CR1 |= TIM_CR1_ARPE;
    //set nvic
    NVIC->ISER[0] = 1<<TIM2_IRQn;
    NVIC_SetPriority(TIM2_IRQn, 0);
}
void TIM3_IRQHandler(void){
  TIM3->SR &= ~TIM_SR_UIF;
  game_loop();
}
void init_tim3(void){
  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
  TIM3->CR1 &= ~TIM_CR1_CEN;
  TIM3->DIER |= TIM_DIER_UIE;
  TIM3->ARR = 800-1;
  TIM3->PSC = 1000-1;
  NVIC->ISER[0] = 1<<TIM3_IRQn;
  NVIC_SetPriority(TIM3_IRQn, 1);
}

void start_tim3(void){
  TIM3->CR1 |= TIM_CR1_CEN;
}

void stop_tim3(void){
  TIM3->CR1 &= ~TIM_CR1_CEN;
  TIM3->SR &= ~TIM_SR_UIF;
}


void init_exti(void){
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN;
  SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI3;
  SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI3_PC;
  EXTI->RTSR |= EXTI_RTSR_TR3;
  EXTI->IMR |= EXTI_IMR_MR3;
  NVIC->ISER[0] = 1<<EXTI2_3_IRQn;
  NVIC_SetPriority(EXTI2_3_IRQn, 3);
}

void EXTI2_3_IRQHandler(void){
  EXTI->PR = EXTI_PR_PR3;
  if(stage == MAIN){
    stage = IN_GAME;
    create_game();
    start_tim3();
  }
  else if(stage == IN_GAME){
    if(state.player){
      fire_player_bullet();
    }
  }
  else if(stage == WIN){
    stage = IN_GAME;
    next_level();
    start_tim3();
  }
  else if(stage == LOSE){
    stage = IN_GAME;
    create_game();
    start_tim3();
  }
}

void setup_timers(void){
  init_tim6();
  init_tim2(10417);
  init_tim3();
}

void setup_io(void){
  LCD_Setup(); // this will call init_lcd_spi()
  init_i2c();
  init_adxl();
  setup_buttons();
  init_dac();
  init_exti();
}
