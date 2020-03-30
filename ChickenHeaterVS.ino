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
#include <PID_v1.h>
#include "routines.h"
#include "basetypes.h"
#include "button.h"

#define FAN_PIN     4
#define HEAT_PIN    5
#define UP_PIN      8
#define DOWN_PIN    9

#define TIME_SLICE_SEC  15
#define TIME_SLICE_MS  TIME_SLICE_SEC * 1000

#define MIN_ACTION_TIME_MS 1000

LiquidCrystal_I2C _lcd(0x27, 20, 4);

double _set, _input, _output;
double Kp = 20, Ki = 1, Kd = 1;

PID _pid(&_input, &_output, &_set, Kp, Ki, Kd, DIRECT);

button_t _upBtn, _downBtn;
filtertimed_t _filteredTemp;
context_t _ctx;

bool _pidValid = false;

unsigned long _sinceLast = 0;

unsigned long _onStart, _offStart;
unsigned long _onTime, _offTime;

action_state_e _state = eDone;
action_e _action;

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

void updateIndicators() {
    _lcd.setCursor(0, 0);
    _lcd.print(_ctx.isHeatOn
        ? "+"
        : " ");

    _lcd.setCursor(0, 1);
    _lcd.print(_ctx.isFanOn
        ? "-"
        : " ");
}

void clearChars() {
    _lcd.setCursor(6, 0);
    _lcd.print("        ");
    _lcd.setCursor(6, 1);
    _lcd.print("        ");
}

unsigned long toMillis(double pidValue) {
    unsigned long result = (unsigned long)(double)(pidValue * TIME_SLICE_MS / 255.0);
    return result < TIME_SLICE_MS
        ? result
        : TIME_SLICE_MS;
}

void setHeat(bool state) {
    digitalWrite(HEAT_PIN, state ? LOW : HIGH);
    _ctx.isHeatOn = state;
}

void setFan(bool state) {
    digitalWrite(FAN_PIN, state ? LOW : HIGH);
    _ctx.isFanOn = state;
}

void perform(action_e action, bool state) {
    switch (action) {
        case eHeat:
            setHeat(state);
            break;
        case eCool:
            setFan(state);
            break;
    }
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

    _ctx.setTemp = 37.75f;
    _ctx.lastSetTemp = 0;
    _ctx.lastReadTemp = 0;

    _ctx.isReadValid = false;

    _ctx.isFanOn = false;
    _ctx.isHeatOn = false;

    Serial.begin(115200);
    while (!Serial) {
        ;
    }

    Serial.println("Up & running");

    _pid.SetOutputLimits(-255, 255);
    _pid.SetMode(AUTOMATIC);
}

// Add the main program code into the continuous loop() function
void loop()
{
    readButton(UP_PIN, &_upBtn, add);
    readButton(DOWN_PIN, &_downBtn, substract);

    updateIndicators();

    float tempActual = getTemperature(analogRead(A2));
    updateFiltertimed(tempActual, &_filteredTemp, updateTemp);

    bool setChanged = _ctx.setTemp != _ctx.lastSetTemp;
    bool readChanged = _ctx.readTemp != _ctx.lastReadTemp;

    /* Wait until temperature correctly read and PID starts to give valid values */
    if (!_pidValid) {
        if (_ctx.isReadValid) {
            _set = _ctx.setTemp;
            _input = _ctx.readTemp;

            _pid.Compute();

            if (_output == -255) {
                return;
            }
            Serial.print("PID valid: ");
            Serial.println(_output);
            _pidValid = true;
        }
        else {
            /* Wait until temperature read and stabilised */
            return;
        }
    }

    unsigned long now = millis();
    unsigned long diff;

    bool wasPlanning = false;
    
    _ctx.lastSetTemp = _ctx.setTemp;
    _ctx.lastReadTemp = _ctx.readTemp;

    if (setChanged || readChanged) {
        clearChars();

        print_temp(_lcd, 0, "Set  ", _ctx.setTemp);
        print_temp(_lcd, 1, "Read ", _ctx.readTemp);
    }
        
    diff = now - _sinceLast;

    /* Plannig part */
    if (_sinceLast == 0 || diff >= TIME_SLICE_MS) {
        _sinceLast = now;
        wasPlanning = true;

        _set = _ctx.setTemp;
        _input = _ctx.readTemp;

        Serial.print(now);
        Serial.print(" read: ");
        Serial.print(_input);
        Serial.print(" *C, set: ");
        Serial.print(_set);
        Serial.print(" *C, ");

        _pid.Compute();
        Serial.print("PID out: ");
        Serial.println(_output);

        /* Assume idle */
        _action = eIdle;
        _state = eDone;

        /* Temp lower than set, needs heating */
        if (_output > 0) {
            _onTime = toMillis(_output);
            _offTime = TIME_SLICE_MS - _onTime;
            if (_onTime > MIN_ACTION_TIME_MS) {
                _state = eOnTime;
                _action = eHeat;
            }
        }
        /* Temp over set, needs cooling */
        else if (_output < 0) {
            _onTime = toMillis(-_output);
            _offTime = TIME_SLICE_MS - _onTime;
            if (_onTime > MIN_ACTION_TIME_MS) {
                _state = eOnTime;
                _action = eCool;
            }
        } 
    }

    /* Executive part */
    if (!wasPlanning) {
        switch (_state) {
        case eOnTime:
            _onStart = now;
            Serial.print(now);
            Serial.print(" ON for: ");
            Serial.println(_onTime);
            perform(_action, true);
            _state = eOffTime;
            break;

        case eOffTime:
            diff = now - _onStart;
            if (diff >= _onTime) {
                _offStart = now;
                Serial.print(now);
                Serial.print(" OFF for: ");
                Serial.println(_offTime);
                perform(_action, false);
                _state = eFinish;
            }
            break;

        case eFinish:
            diff = now - _offStart;
            if (diff >= _offTime) {
                Serial.print(now);
                Serial.println(" Idle");
                _state = eDone;
            }
            break;
        }
    }
}
