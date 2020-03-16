#pragma once

typedef struct {
    int current;
    int valid;
    unsigned long changedAt;
} button_t;

typedef struct {
    float current;
    float valid;
    unsigned long changedAt;
} filtertimed_t;

typedef struct {
    float setTemp;
    float lastSetTemp;
    
    float readTemp;
    float lastReadTemp;
    bool isReadValid;

    bool isFanOn;
    bool isHeatOn;
} context_t;