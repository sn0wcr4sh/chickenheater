/*
    Name:       ChickenHeaterVS.ino
    Created:	3/16/2020 7:33:07 AM
    Author:     Roman
*/

/*
    PIN to function allocation

    Temperature sensor  - A2

    Relay 1 (heating)   - D5
    Relay 2 (fan)       - D4
    Relay 3             - D6
    Relay 4             - D7

    Up button           - D8
    Down button         - D9

*/

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "routines.h"
#include "basetypes.h"
#include "button.h"

#define FAN_PIN     4
#define HEAT_PIN    5
#define UP_PIN      8
#define DOWN_PIN    9

LiquidCrystal_I2C _lcd(0x27, 20, 4);

button_t _upBtn, _downBtn;
filtertimed_t _filteredTemp;
context_t _ctx;

void add(float val) {
    _ctx.setTemp += val;
}

void substract(float val) {
    _ctx.setTemp -= val;
}

void updateTemp(float val) {
    _ctx.readTemp = val;
    _ctx.isReadValid = true;
}

// The setup() function runs once each time the micro-controller starts
void setup()
{
    _lcd.init();
    _lcd.init();

    _lcd.backlight();

    for (int i = 4; i <= 7; i++) {
        pinMode(i, OUTPUT);
        digitalWrite(i, HIGH);
    }

    pinMode(UP_PIN, INPUT);
    pinMode(DOWN_PIN, INPUT);

    _upBtn.current = 0;
    _upBtn.valid = 0;
    _upBtn.changedAt = millis();

    _downBtn.current = 0;
    _downBtn.valid = 0;
    _downBtn.changedAt = millis();

    _ctx.setTemp = 37.7f;
    _ctx.lastSetTemp = 0;
    _ctx.lastReadTemp = 0;

    _ctx.isReadValid = false;

    _ctx.isFanOn = false;
    _ctx.isHeatOn = false;
}

void clearChars() {
    _lcd.setCursor(0, 0);
    _lcd.print(" ");
    _lcd.setCursor(0, 1);
    _lcd.print(" ");

    _lcd.setCursor(6, 0);
    _lcd.print("        ");
    _lcd.setCursor(6, 1);
    _lcd.print("        ");
}

// Add the main program code into the continuous loop() function
void loop()
{
    readButton(UP_PIN, &_upBtn, add);
    readButton(DOWN_PIN, &_downBtn, substract);

    float tempActual = getTemperature(analogRead(A2));
    updateFiltertimed(tempActual, &_filteredTemp, updateTemp);

    bool setChanged = _ctx.setTemp != _ctx.lastSetTemp;
    bool readChanged = _ctx.readTemp != _ctx.lastReadTemp;

    if (setChanged || readChanged) {
        _ctx.lastSetTemp = _ctx.setTemp;
        _ctx.lastReadTemp = _ctx.readTemp;

        clearChars();
        if (_ctx.isFanOn) {
            _lcd.setCursor(0, 1);
            _lcd.print("-");
        }
        if (_ctx.isHeatOn) {
            _lcd.setCursor(0, 0);
            _lcd.print("+");
        }

        print_temp(_lcd, 0, "Set  ", _ctx.setTemp);
        print_temp(_lcd, 1, "Read ", _ctx.readTemp);    

        /* Only do stuff when temp read */
        if (_ctx.isReadValid) {
            /* Fan hysteresis: go off when at or less than set (heater is cooling slowly after goes off) */
            if (_ctx.isFanOn) {
                if (_ctx.readTemp <= _ctx.setTemp) {
                    digitalWrite(FAN_PIN, HIGH);
                    _ctx.isFanOn = false;
                }
            }
            else {
                if (_ctx.readTemp > _ctx.setTemp + 1.0f) {
                    digitalWrite(FAN_PIN, LOW);
                    _ctx.isFanOn = true;
                }
            }

            if (!_ctx.isFanOn) {    // never heating when fan is on
                if (_ctx.isHeatOn) {
                    if (_ctx.readTemp >= _ctx.setTemp) {
                        /* Go off directly at set, heater will be hot some time after anyway */
                        digitalWrite(HEAT_PIN, HIGH);
                        _ctx.isHeatOn = false;
                    }
                }
                else {
                    if (_ctx.readTemp <= _ctx.setTemp - 1.0f) {
                        digitalWrite(HEAT_PIN, LOW);
                        _ctx.isHeatOn = true;
                    }
                }
            }
        }
    }
}
