#include <xc.h>

#include "config_877A.h"

#include "display7s.h"
#include "i2c.h"
#include "eeprom_ext.h"
#include "adc.h"
#include "itoa.h"

#ifndef _16F777
__EEPROM_DATA(0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77);
__EEPROM_DATA(0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF);
#endif

unsigned char state = 'O'; // OFF = F, ON = N, Setting = S
unsigned char heater_on = 0;
unsigned char cooler_on = 0;
unsigned char n_blinks = 0;

unsigned int measured_temperature = 55; // avg of max and min temps
unsigned int set_temperature = 60;

unsigned char t_1sec = 0;
unsigned char ssd_mask = 1;

unsigned char i_7sd = 0;

unsigned int temps[10];
int temps_iterator = 0;

void activate_heater()
{
  if (!heater_on)
  {
    PORTCbits.RC5 = 1;
    heater_on = 1;
    RB6 = 1;
  }
}

void deactivate_heater()
{
  if (state == 'F' || set_temperature == measured_temperature || cooler_on)
  {
    PORTCbits.RC5 = 0;
    heater_on = 0;
    if (!cooler_on)
      RB6 = 0;
  }
}

void activate_cooler()
{
  if (!cooler_on)
  {
    PORTCbits.RC2 = 1;
    cooler_on = 1;
    RB6 = 1;
  }
}

void deactivate_cooler()
{
  if (state == 'F' || set_temperature == measured_temperature || heater_on)
  {
    PORTCbits.RC2 = 0;
    cooler_on = 0;
    if (!heater_on)
      RB6 = 0;
  }
}

void adc_sample()
{
  adc_init();
  temps[temps_iterator++] = (adc_amostra(2) * 10) / 20;
  measured_temperature = calc_avg();
  temps_iterator = (temps_iterator > 9) ? 0 : temps_iterator;
}

int calc_avg()
{
  char i;
  unsigned int avg = 0;
  for (i = 0; i < 10; i++)
  {
    avg += temps[i];
  }
  return avg / 10;
}

void interrupts_init()
{
  // Enabling global and peripheral interrupt masks
  INTCONbits.GIE = 1;
  INTCONbits.PEIE = 1;
  // Enabling timer 0 and timer 1 interrupts
  INTCONbits.TMR0IE = 1;
  PIE1bits.TMR1IE = 1;
  // enabling interrupts on RB0 as on/off button
  INTCONbits.INTE = 1; // check on INTF in ISR and clear it
}

void timer0_init()
{
  // 256 Prescalar
  OPTION_REG = 0b00000111;
  // RegValue = 256-(Delay * Fosc)/(Prescalar*4)), 10 ms
  TMR0 = 61;
}

void timer1_init()
{

  // RegValue = 65536-(Delay * Fosc)/(Prescalar*4)), 100 ms
  TMR1L = 0xdc;
  TMR1H = 0x0b;

  // Prescalar = 8
  T1CONbits.T1CKPS0 = 1;
  T1CONbits.T1CKPS1 = 1;

  // Turn On timer 1
  T1CONbits.TMR1ON = 1;
}

void interrupt ISR()
{
  // RB0 was pressed
  if (INTF)
  {
    state = state == 'O' || state == 'S' ? 'F' : 'O';
    INTF = 0;
    return;
  }

  // If on or in setting mode
  if (state != 'F')
  {
    // Enable LCD if on
    if (state == 'O')
    {
      ssd_mask = 0xff;
    }

    // If timer 0
    if (TMR0IF)
    {

      if (i_7sd)
      {

        PORTA = 0x10;
        if (state == 'S')
          PORTD = ssd_mask & (display7s((unsigned char)(set_temperature % 10)));
        else
          PORTD = ssd_mask & (display7s((unsigned char)(measured_temperature % 10)));
      }
      else
      {
        PORTA = 0x08;
        if (state == 'S')
          PORTD = ssd_mask & (display7s((unsigned char)(set_temperature / 10)));
        else
          PORTD = ssd_mask & (display7s((unsigned char)(measured_temperature / 10)));
      }
      i_7sd = !i_7sd;
      TMR0 = 61;
      TMR0IF = 0;
      // reload, clear, and begin again
    }

    // if timer 1
    if (TMR1IF)
    {
      adc_sample();
      if (++t_1sec == 10)
      {
        t_1sec = 0;
        if (heater_on)
        {
          PORTB ^= (1 << 6);
        }
        if (state == 'S')
        {
          ssd_mask = (ssd_mask == 0xff) ? 0x00 : 0xff;
          if ((++n_blinks) == 10)
          {
            e2pext_w(10, set_temperature);
            n_blinks = 0;
            state = 'O';
          }
        }
      }
      TMR1H = 0x0b;
      TMR1L = 0xdc;
      PIR1bits.TMR1IF = 0;
      T1CONbits.TMR1ON = 1;
    }
  }
  // If off
  else
  {
    PORTD = 0x00;
  }
}

void set_temp_exteeprom_check()
{
  // Check the location 11 for our key "0x33"
  // if location 11 contains the key, we load the saved temperature in location 10
  // else, write set_temp (60 by default) to location 10, and write the key in location 11
  unsigned char read_char;
  // TRISB = 0x03;
  read_char = e2pext_r(11);
  if (read_char == 0x33)
  {
    set_temperature = e2pext_r(10);
  }
  else
  {
    e2pext_w(10, set_temperature);
    e2pext_w(11, 0x33);
  }
}

void main()
{
  // Calculated difference between set and measured temperatures
  int difference;
  // First 3 bits used by ADC, others output to control 7SDs
  TRISA = 0x07;
  // 6 Heating led, 7 cooling led, B0:B2 as inputs
  TRISB = 0x07;
  // All output to prevent unwanted inputs.
  TRISC = 0x00;
  // 7SD connected to PORTD
  TRISD = 0x00;
  // Unused ADC channels
  TRISE = 0x00;

  // Initializing drivers
  i2c_init();
  timer1_init();
  timer0_init();
  // Enabling required interrupts
  interrupts_init();
  // Reloading/loading of set temperature
  set_temp_exteeprom_check();
  // Setting PCFG3:PCFG0 bits, A/D Port Configuration Control bits
  ADCON1 = 0x0F;

  while (1)
  {
    if (state != 'F')
    {
      if (!PORTBbits.RB1)
      {
        n_blinks = 0;
        while (!PORTBbits.RB1)
          ;

        if (state == 'S' && set_temperature >= 40)
        {
          GIE = 0;
          set_temperature -= 5;
          GIE = 1;
        }
        else
          state = 'S';
      }
      else if (!PORTBbits.RB2)
      {
        n_blinks = 0;
        while (!PORTBbits.RB2)
          ;
        if (state == 'S' && set_temperature <= 70)
        {
          GIE = 0;
          set_temperature += 5;
          GIE = 1;
        }
        else
          state = 'S';
      }
      // +ve diff, Heater On
      // -ve diff, Cooler On
      difference = set_temperature - measured_temperature;
      if (difference >= 5)
      {
        activate_heater();
        deactivate_cooler();
      }
      else if (difference <= -5)
      {
        deactivate_heater();
        activate_cooler();
      }
      else
      {
        deactivate_heater();
        deactivate_cooler();
      }
    }
    else
    {
      deactivate_heater();
      deactivate_cooler();
    }
  }
}