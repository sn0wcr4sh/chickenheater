#pragma once

typedef enum {
    eHeat,
    eCool,
    eIdle
} action_e;

typedef enum {
    eOnTime,
    eOffTime,
    eFinish,
    eDone
} action_state_e;

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