// button.h

#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "basetypes.h"

void readButton(int pinId, button_t* button, void (*callback)(float val));

