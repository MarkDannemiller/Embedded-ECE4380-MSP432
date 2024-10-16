#ifndef TICKERS_H
#define TICKERS_H

#include "p100.h"

#define MAX_TICKERS 16
#define TICKER_TICK_MS 10  // 10 ms granularity

typedef struct {
    bool active;
    uint32_t initialDelay;   // In ticks (10 ms units)
    uint32_t period;         // In ticks (10 ms units)
    int32_t count;           // Number of times to repeat (-1 for infinite)
    uint32_t currentDelay;   // Current delay counter
    char payload[BUFFER_SIZE];
} Ticker;

extern Ticker tickers[MAX_TICKERS];

// Function prototypes
void init_tickers();
void print_all_tickers()
void ticker_timer_callback(Timer_Handle myHandle, int_fast16_t status);
void process_tickers();

#endif // TICKERS_H
