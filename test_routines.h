#pragma once
#ifdef TEST

#include <cstdint>

uint8_t pins();
uint8_t pin(uint8_t idx);
void setCounter(uint8_t number, uint32_t value);
void initialize();
uint32_t millis();
void systemSleep();

#endif // TEST
