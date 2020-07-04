#include <xc.h>

#include "config_877A.h"
#include "display7s.h"
#include "i2c.h"
#include "eeprom_ext.h"
#include "adc.h"

// Heater and cooler state variables
unsigned char heater_on = 0;
unsigned char cooler_on = 0;
// Defining pins for heating and cooling elements and the indicator LED
#define HEATING_COOLING_LED RB6
#define HEATING_ELEMENT_PIN RC5
#define COOLING_ELEMENT_PIN RC2

// System's state definitions and state variable
#define ON_STATE 'O'
#define OFF_STATE 'F'
#define SETTING_STATE 'S'
unsigned char state = OFF_STATE;

// Mask to turn on 7SDs
#define ON_7SD 0xff

/* 
ON/OFF -> RB0
UP -> R2
Down -> R1
*/
#define UP_BTN RB2
#define DOWN_BTN RB1

// To track number of blinks when setting the temperature
unsigned char n_blinks = 0;

// Avg of last 10 measured temperatures, initially equals avg of max and min possible set temperatures
unsigned int avg_measured_temperature = 55;

// The temperature set by the user, loaded from external E2PROM on startup if available, or default(60)
unsigned int set_temperature = 60;

// Counter to measure 1 sec, incremented by 1 every 100ms in ISR of timer 1, and is reset when it reaches 10
unsigned char t_1sec = 0;

// Mask for blinking LED in the setting state
unsigned char _7sd_mask = ON_7SD;

// Toggled every 10ms to track which 7SD to output to at some given time
unsigned char i_7sd = 0;

// Array to track last 10 temperature measurements
unsigned int temps[10];

/* 
 iterator to track the index of the measurement to update in the "temps" array next
 Reset when it reaches 10
*/
int temps_iterator = 0;

// Function Declarations

void activate_heater();
void deactivate_heater();
void activate_cooler();
void deactivate_cooler();
unsigned int calc_avg();
void show_7sd();
void _1s_handler();
void adc_sample_and_update_average();
void interrupts_init();
void timer0_init();
void timer1_init();
void interrupt ISR();
void set_temp_exteeprom_check();
void main();

// Activate heating element, its LED, and set the heating state variable
void activate_heater()
{
  if (!heater_on)
  {
    HEATING_ELEMENT_PIN = 1;
    heater_on = 1;
    HEATING_COOLING_LED = 1;
  }
}

// Activate heating element, its LED, and clear the heating state variable
void deactivate_heater()
{
  if (state == OFF_STATE || set_temperature == avg_measured_temperature || cooler_on)
  {
    HEATING_ELEMENT_PIN = 0;
    heater_on = 0;
    if (!cooler_on)
      HEATING_COOLING_LED = 0;
  }
}

// Activate cooling element, its LED, and set the cooling state variable
void activate_cooler()
{
  if (!cooler_on)
  {
    COOLING_ELEMENT_PIN = 1;
    cooler_on = 1;
    HEATING_COOLING_LED = 1;
  }
}

// Dectivate cooling element, its LED, and clear the cooling state variable
void deactivate_cooler()
{
  if (state == OFF_STATE || set_temperature == avg_measured_temperature || heater_on)
  {
    COOLING_ELEMENT_PIN = 0;
    cooler_on = 0;
    if (!heater_on)
      HEATING_COOLING_LED = 0;
  }
}

// Calculates the average of values in the "temps" array and returns it.
unsigned int calc_avg()
{
  char i;
  unsigned int avg = 0;
  for (i = 0; i < 10; i++)
  {
    avg += temps[i];
  }
  return avg / 10;
}

/*

Writes to 7SD then updates i_7sd to write to the next 7sd when called the next time.

*/
void show_7sd()
{
  switch (i_7sd)
  {
  case 0:
    PORTA = 0x20;
    PORTD = _7sd_mask & (display7s(12));
    i_7sd = 1;
    break;
  case 1:
    PORTA = 0x10;
    if (state == SETTING_STATE)
      PORTD = _7sd_mask & (display7s((unsigned char)(set_temperature % 10)));
    else
      PORTD = _7sd_mask & (display7s((unsigned char)(avg_measured_temperature % 10)));
    i_7sd = 2;
    break;
  case 2:
    PORTA = 0x08;
    if (state == SETTING_STATE)
      PORTD = _7sd_mask & (display7s((unsigned char)(set_temperature / 10)));
    else
      PORTD = _7sd_mask & (display7s((unsigned char)(avg_measured_temperature / 10)));
    i_7sd = 0;
    break;
  default:
    break;
  }
}

/*
1- Toggles heating/cooling LED if needed.
2- In settings mode, blinks 7sd every one second, blink count is reset if RB1 or RB2 is pressed.
3- In settings mode, if 7sd is blinked 5 times without RB1 or RB2 being pressed, save the set temperature to the external e2prom. 
*/
void _1s_handler()
{
  if (heater_on)
  {
    PORTB ^= (1 << 6); // Toggle RB6
  }
  if (state == SETTING_STATE)
  {
    _7sd_mask = (_7sd_mask == ON_7SD) ? 0x00 : ON_7SD;
    if ((++n_blinks) == 10)
    {
      e2pext_w(10, set_temperature);
      n_blinks = 0;
      state = ON_STATE;
    }
  }
}

/* 
1 - Measures the temperature using the ADC
2 - updates "temps" array and its iterator
3 - updates "avg_measured_temperature" with the new average of the last 10 temperatures
*/
void adc_sample_and_update_average()
{
  adc_init();
  temps[temps_iterator++] = (adc_sample(2) * 10) / 20;
  avg_measured_temperature = calc_avg();
  temps_iterator = (temps_iterator > 9) ? 0 : temps_iterator;
}

// Enables interrupts on timer0, timer1, and RB0
void interrupts_init()
{
  // Enabling global and peripheral interrupt masks
  GIE = 1;
  PEIE = 1;

  // Enabling timer 0 and timer 1 interrupts
  TMR0IE = 1;
  TMR1IE = 1;

  // enabling interrupts on RB0 as on/off button
  INTE = 1; // check on INTF in ISR and clear it
}

// Configuring timer0 to provide 10ms delay
void timer0_init()
{
  // Prescalar = 256
  OPTION_REG = 0b00000111;

  // TMR0 = 256-(Delay * Fosc)/(Prescalar*4)), delay = 10 ms and Fosc = 20MHz
  TMR0 = 61;
}

// Configuring timer1 to provide 100ms delay
void timer1_init()
{

  // Prescalar = 8
  T1CKPS0 = 1;
  T1CKPS1 = 1;

  // TMR1L concatenated with TMR1H = 65536-(Delay * Fosc)/(Prescalar*4)), delay = 100 ms and Fosc = 20MHz
  TMR1L = 0xdc;
  TMR1H = 0x0b;

  // Turn On timer 1
  TMR1ON = 1;
}

/*
ISR runs when any enabled interrupt happens
 
 1- RB0 Interrupts are used to switch between the system states (ON, OFF, and Setting)

 2- Timer0 interrupts are used to switch between writing to the two 7SDs every 10ms to create the illusion 
    that both are working at the same time

 3- Timer1 interrupts are used for
    3.1 - Sampling of the temperature every interrupt (100ms)
    3.2 - Blinking of the displays every 10 interrupts (1s) in setting mode by toggeling _7sd_mask
    3.3 - Blinking heating element LED every 10 interrupts (1s) when heating element is on
*/
void interrupt ISR()
{
  // RB0 was pressed
  if (INTF)
  {
    state = state == ON_STATE || state == SETTING_STATE ? OFF_STATE : ON_STATE;
    INTF = 0;
    return;
  }

  // If on or setting mode
  if (state != OFF_STATE)
  {
    // Enable LCD if on
    if (state == ON_STATE)
    {
      _7sd_mask = ON_7SD;
    }

    // If timer 0
    if (TMR0IF)
    {
      show_7sd();
      // reload and clear interrupt flag
      TMR0 = 61;
      TMR0IF = 0;
    }

    // if timer 1
    if (TMR1IF)
    {
      adc_sample_and_update_average();
      if (++t_1sec == 10)
      {
        t_1sec = 0;
        _1s_handler();
      }
      // reload, clear interrupt flag, and restart timer
      TMR1H = 0x0b;
      TMR1L = 0xdc;
      TMR1IF = 0;
      TMR1ON = 1;
    }
  }
  // If off
  else
  {
    // Switch Off the 7SDs
    PORTD = 0x00;
  }
}

/* 
  Check the location 11 for our key "0x33"
  if location 11 contains the key, we load the saved temperature from location 10
  else, write set_temp (60 by default) to location 10, and write the key in location 11
*/
void set_temp_exteeprom_check()
{
  unsigned char read_char;
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

  /*
    1 - Monitors Up and down button presses (handling debouncing effect)
    2 - Monitors Temperature and activates/deactivates actuators accordingly
  */
  while (1)
  {
    if (state != OFF_STATE)
    {
      if (!DOWN_BTN)
      {
        n_blinks = 0;
        while (!DOWN_BTN)
          ;

        if (state == SETTING_STATE && set_temperature >= 40)
        {
          GIE = 0;
          set_temperature -= 5;
          GIE = 1;
        }
        else
          state = SETTING_STATE;
      }
      else if (!UP_BTN)
      {
        n_blinks = 0;
        while (!UP_BTN)
          ;
        if (state == SETTING_STATE && set_temperature <= 70)
        {
          GIE = 0;
          set_temperature += 5;
          GIE = 1;
        }
        else
          state = SETTING_STATE;
      }
      // +ve diff, Heater On
      // -ve diff, Cooler On
      difference = set_temperature - avg_measured_temperature;
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