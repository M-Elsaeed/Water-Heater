#include <xc.h>

#ifdef _18F452
#include "config_452.h"
#endif
#ifdef _18F4620
#include "config_4620.h"
#endif
#ifdef _18F4550
#include "config_4550.h"
#endif
#ifdef _16F877A
#include "config_877A.h"
#endif
#ifdef _16F777
#include "config_777.h"
#endif

#include "atraso.h"
#include "lcd.h"
#include "display7s.h"
#ifndef _18F4550
#include "i2c.h"
#include "eeprom_ext.h"
#include "rtc.h"
#endif
#include "serial.h"
#ifndef _16F777
#include "eeprom.h"
#endif
#include "adc.h"
#include "itoa.h"
#include "teclado.h"

#ifndef _16F777
__EEPROM_DATA(0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77);
__EEPROM_DATA(0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF);
#endif

int measured_temperature = 0;
int set_temperature = 60;
char i_7sd = 0;
char t_1sec = 0;
char state = 'F'; // OFF = F, ON = N, Setting = S
unsigned char temps[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
char temps_iterator = 0;

void activate_heater()
{
  // while (PORTBbits.RB1)
  //   ;
  // while (!PORTBbits.RB1)
  //   ;
  // Heater connected to RC5 1 on, 0 off

  PORTCbits.RC5 = 1;

  // while (PORTBbits.RB1)
  //   ;
  // while (!PORTBbits.RB1)
  //   ;

  PORTCbits.RC5 = 0;
}

void activate_cooler()
{
  // while (PORTBbits.RB1)
  //   ;
  // while (!PORTBbits.RB1)
  //   ;
  // Cooler connected to RC2
  // 1 on, 0 off
  PORTCbits.RC2 = 1;
  // while (PORTBbits.RB1)
  //   ;
  // while (!PORTBbits.RB1)
  //   ;
  PORTCbits.RC2 = 0;
}

void adc_sample()
{
  // unsigned int tmpi;
  // unsigned char i;
  // char str[6];
  // // 2,3

  // lcd_cmd(L_CLR);
  // lcd_cmd(L_L1);
  // lcd_str("ADC");
  TRISA = 0x07;

  adc_init();

  temps[temps_iterator++] = (adc_amostra(2) * 10) / 2;
  measured_temperature = calc_avg();
  temps_iterator = temps_iterator > 9 ? 0 : temps_iterator;

  // for (i = 0; i < 100; i++)
  // {
  // tmpi = (adc_amostra(2) * 10) / 2;
  // lcd_cmd(L_L2);
  // itoa(tmpi, str);
  // lcd_str(str);
  // atraso_ms(20);
  // }
}

void ssd(int num)
{
  unsigned char i;
  unsigned char tmp;
  // TRISA = 0xC3;
  while (1)
  {
    PORTA = 0x20;
    PORTD = display7s(12);
    atraso_ms(10);
    PORTA = 0x10;
    PORTD = display7s(num % 10);
    atraso_ms(10);
    PORTA = 0x08;
    PORTD = display7s(num / 10);
    atraso_ms(10);
  }
}

void eprom()
{
  unsigned char tmp;
  TRISB = 0x03;
  tmp = e2pext_r(10);
  e2pext_w(10, 0xA5);
}

int calc_avg()
{
  char i;
  int avg = 0;
  for (i = 0; i < 10; i++)
  {
    avg += temps[i];
  }
  return avg / 10;
}

void interrupt tmr0()
{
  if (i_7sd)
  {

    PORTA = 0x10;
    PORTD = display7s(set_temperature % 10);
  }
  else
  {
    PORTA = 0x08;
    PORTD = display7s(set_temperature / 10);
  }
  i_7sd = !i_7sd;
  // reload, clear, and begin again
}

void interrupt tmr1()
{
  adc_sample();
  if(++t_1sec == 10){
    t_1sec = 0;
    // turn on led for heating
    // flash 7sd if settings mode
  }
  TMR1H = 0x0b;
  TMR1L = 0xdc;
  PIR1bits.TMR1IF = 0;
  T1CONbits.TMR1ON = 1;
}

void intr_init()
{
  INTCONbits.GIE = 1;
  INTCONbits.PEIE = 1;
  PIE1bits.TMR1IE = 1;
}

void timer1_init()
{

  // TMR1IF check flag in PIR1, write to 0 in ISR
  // 100ms
  TMR1L = 0xdc;
  TMR1H = 0x0b;

  // Prescalar = 8
  T1CONbits.T1CKPS0 = 1;
  T1CONbits.T1CKPS1 = 1;

  // Turn On timer 1
  T1CONbits.TMR1ON = 1;
}

void main()
{

  unsigned char i;
  unsigned char tmp;
  unsigned int tmpi;

  char str[6];

  TRISA = 0xC3;
  TRISB = 0x01;
  TRISC = 0x01;
  TRISD = 0x00;
  TRISE = 0x00;

  lcd_init();
  i2c_init();
  serial_init();
  adc_init();
  intr_init();
  timer1_init();
  ADCON1 = 0x0F;
  CMCON = 0x07;
  //dip
  TRISB = 0x03;
  lcd_cmd(L_CLR);
  lcd_cmd(L_L1);
  lcd_str("welcome mfrs");
  lcd_cmd(L_L2);
  lcd_str("Press. RB1");
  while (PORTBbits.RB1)
    ;

  // eprom();
  // activate_heater();
  // activate_cooler();
  // adc_sample();
  // ssd(39);
  while (1)
    ;
}