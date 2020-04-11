/* Audio Sample Player v2

   David Johnson-Davies - www.technoblogy.com - 23rd October 2017
   ATtiny85 @ 8 MHz (internal oscillator; BOD disabled)
      
   CC BY 4.0
   Licensed under a Creative Commons Attribution 4.0 International license: 
   http://creativecommons.org/licenses/by/4.0/
*/

/* Direct-coupled capacitorless output */
#include <Arduino.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#define adc_disable()  (ADCSRA &= ~(1<<ADEN))

// Audio encoded as unsigned 8-bit, 8kHz sampling rate
#include "sound_progmem.h"

unsigned int p = 0;



#define F_CPU 8000000
#define F_S 8000 //samplerate
#define PRESCALE 8
//F_CPU  = F_S * PRESCALE * (OCR_COUNT + 1)
#define FLOOR(x, align)     (((x) / (align)) * (align))

//this doesnt work for some bizarre reason, precompiler/compiler shenanigans maybe
#define OCR_COUNT FLOOR( (F_CPU/(PRESCALE * F_S)) - 1, 2) 

#define VALUE_TO_STRING(x) #x
#define VALUE(x) VALUE_TO_STRING(x)
#define VAR_NAME_VALUE(var) #var "="  VALUE(var) 

#pragma message (VAR_NAME_VALUE(OCR_COUNT))

void setup () {
  // Enable 64 MHz PLL and use as source for Timer1
  PLLCSR = 1<<PCKE | 1<<PLLE;
  OSCCAL = 255;
 
  // Set up Timer/Counter1 for PWM output
  TIMSK = 0;                              // Timer interrupts OFF
  TCCR1 = 1<<PWM1A | 2<<COM1A0 | 1<<CS10; // PWM A, clear on match, 1:1 prescale
  GTCCR = 1<<PWM1B | 2<<COM1B0;           // PWM B, clear on match
  OCR1A = 128; OCR1B = 128;               // 50% duty at start

  // Set up Timer/Counter0 for 8kHz interrupt to output samples.
  //We now output at 22KHz
  TCCR0A = 3<<WGM00;                      // Fast PWM
  TCCR0B = 1<<WGM02 | 2<<CS00;            // 1/8 prescale
  TIMSK = 1<<OCIE0A;                      // Enable compare match
  OCR0A = 100;                            // Now at ~22050 KHz
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  pinMode(4, OUTPUT);
  pinMode(1, OUTPUT);
 
}

void loop() { // loop over all possible duty cycles and set the compare registers accordingly

}


float getBit(uint8_t n, uint8_t byte) {
  n = 7 - n;
  int bit = ((byte & (1 << n)) >> n);
  return (float) (bit == 0 ? -1 : 1);
}

uint8_t current_byte = 0;
float y = 0;
// Sample interrupt
ISR (TIMER0_COMPA_vect) {
  int inbyte_p = p%8;
  int byte_p = p/8;
 
  if (inbyte_p == 0 ) {
        current_byte = pgm_read_byte(&sound_progmem_pdm[byte_p]);
      }
  float x = getBit(inbyte_p,current_byte);
    
  p++;
  //Low-pass filter
  y = (0.875)*y + x/8.0;
  int sample = (int)( y * 100.0);
  
  OCR1A = sample; OCR1B = sample ^ 255;
  
  if (p == sound_progmem_pdm_len*8) {
    // p=0; 
   TIMSK = 0;
   adc_disable();
   sleep_enable();
   sleep_cpu();  // 1uA
  }
}
