#ifndef CALLBACK_H
#define CALLBACK_H

#include "p100.h"

#define MAX_CALLBACKS 3  // Indices 0, 1, 2

typedef struct {
    int count;                  // Number of times to execute (-1 for infinite)
    char payload[BUFFER_SIZE];  // Command to execute
} CommandCallback;

extern CommandCallback callbacks[MAX_CALLBACKS];

void print_all_callbacks();

void timer0Callback_fxn(Timer_Handle handle, int_fast16_t status);
void timer0SWI(UArg arg0, UArg arg1);

void sw1Callback_fxn(uint_least8_t index);
void sw1SWI(UArg arg0, UArg arg1);

void sw2Callback_fxn(uint_least8_t index);
void sw2SWI(UArg arg0, UArg arg1);

#endif  // CALLBACK_H
