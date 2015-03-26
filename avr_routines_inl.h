#pragma once

#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

void systemSleep()
{
  // Disable ADC & comparator
  ADCSRA &= ~(1 << ADEN);
  ACSR |= (1 << ACD);
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  sleep_enable();
  sleep_cpu();
}

ISR(PCINT0_vect)
{
    // noop, just get up mc
}

uint8_t pin(uint8_t idx)
{
  return !!(PINB & _BV(idx));
}

void setCounter(uint8_t, uint32_t)
{
    digitalWrite(4, 1);
    delay(1000);
    digitalWrite(4, 0);        
}

