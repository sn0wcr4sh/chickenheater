// 
// 
// 

#include "button.h"

#define FILTERTIME 100

void readButton(int pinId, button_t* button, void (*callback)(float val)) {
    int val = digitalRead(pinId);

    if (val != button->current) {
        button->current = val;
        button->changedAt = millis();
        return;
    }

    if (button->current != button->valid) {
        unsigned long now = millis();
        unsigned long diff = now - button->changedAt;
        if (diff > FILTERTIME) {
            button->valid = button->current;
            callback(
                button->valid != 0
                ? 0
                : 0.25f);
        }
    }
}
