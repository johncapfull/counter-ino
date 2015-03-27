#pragma once

#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

namespace {

void systemSleep()
{
  // Disable ADC & comparator
  ADCSRA &= ~(1 << ADEN);
  ACSR |= (1 << ACD);
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  sleep_enable();
  sleep_cpu();
}

} // namespace
