// routines.h

#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <LiquidCrystal_I2C.h>
#include "basetypes.h"

void updateFiltertimed(float newValue, filtertimed_t* temp, void (*callback)(float val));
float getTemperature(int adcValue);
void print_temp(LiquidCrystal_I2C lcd, int row, String prefix, float value);