#include "routines.h"

#define ADC_OFFSET -0.2f
#define TEMP_FILTERTIME 50
#define TEMP_EPSILON 0.5f

#define TABLE_SIZE  50

float CONV_TABLE[][2] =
{
    { 0.56, 68057 },
    { 1.67, 64129 },
    { 2.78, 60451 },
    { 3.89, 57005 },
    { 5.00, 53777 },
    { 6.11, 50750 },
    { 7.22, 47912 },
    { 8.33, 45249 },
    { 9.44, 42750 },
    { 10.56, 40383 },
    { 11.67, 38180 },
    { 12.78, 36111 },
    { 13.89, 34165 },
    { 15.00, 32336 },
    { 16.11, 30615 },
    { 17.22, 28996 },
    { 18.33, 27472 },
    { 19.44, 26037 },
    { 20.56, 24674 },
    { 21.67, 23400 },
    { 22.78, 22200 },
    { 23.89, 21068 },
    { 25.00, 20001 },
    { 26.11, 18994 },
    { 27.22, 18043 },
    { 28.33, 17145 },
    { 29.44, 16297 },
    { 30.56, 15488 },
    { 31.67, 14731 },
    { 32.78, 14016 },
    { 33.89, 13339 },
    { 35.00, 12699 },
    { 36.11, 12092 },
    { 37.22, 11519 },
    { 38.33, 10975 },
    { 39.44, 10461 },
    { 40.56, 9969 },
    { 41.67, 9507 },
    { 42.78, 9069 },
    { 43.89, 8654 },
    { 45.00, 8260},
    { 46.11, 7886},
    { 47.22, 7531 },
    { 48.33, 7194 },
    { 49.44, 6874 },
    { 50.56, 6567 },
    { 51.67, 6278 },
    { 52.78, 6004 },
    { 53.89, 5742 },
    { 55.00, 5494 },
};

void updateFiltertimed(float val, filtertimed_t* temp, void (*callback)(float val)) {
    
    float valDiff = temp->current - val;
    if (valDiff < 0)
        valDiff *= -1;

    if (valDiff > TEMP_EPSILON) {
        temp->current = val;
        temp->changedAt = millis();
        return;
    }

    if (temp->current != temp->valid) {
        unsigned long now = millis();
        unsigned long diff = now - temp->changedAt;
        if (diff > TEMP_FILTERTIME) {
            temp->valid = temp->current;
            callback(temp->valid);
        }
    }
}

// -1000 indicates colder than detectable, 1000 hotter than detectable
float getTemperature(int adcValue) {
    float vo = ((float)adcValue * 5.0 / 1023.0) + ADC_OFFSET;
    long rt = (vo * 20000) / (5 - vo);

    /* Check if not freezing (outside lowest value in table) */
    if (rt > CONV_TABLE[0][1])
        return -1000;
    
    for (int i = 1; i < TABLE_SIZE; i++) {
        if (rt >= CONV_TABLE[i][1]) {
            /* Interval to which Rt belongs found */

            float rProportion = (rt - CONV_TABLE[i][1]) / (CONV_TABLE[i - 1][1] - CONV_TABLE[i][1]);
            float tRange = CONV_TABLE[i][0] - CONV_TABLE[i - 1][0];

            float result = CONV_TABLE[i][0] - (tRange * rProportion);
            
            return result;
        }
    }

    /* Outside highest value in table (Abu Dhabi in hot summer) */
    return 1000;
}

void print_temp(LiquidCrystal_I2C lcd, int row, String prefix, float value) {
    String text = prefix;
    
    if (value <= -1000)
        text += "< 0";
    else if (value >= 1000)
        text += "> 50";
    else
        text += value;

    text += " ";
    text += (char)223;
    text += "C";
    lcd.setCursor(1, row);
    lcd.print(text);
}
