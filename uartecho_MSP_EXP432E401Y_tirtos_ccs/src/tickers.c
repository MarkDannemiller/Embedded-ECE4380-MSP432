#include <string.h>
#include <stdlib.h>
#include "tickers.h"
#include "tasks.h"   // For execute_payload()
#include "callback.h" // For BUFFER_SIZE
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/BIOS.h>
#include <xdc/runtime/System.h>


// Global array of tickers
Ticker tickers[MAX_TICKERS];

// Timer handle for tickers
Timer_Handle tickerTimer;
Timer_Params tickerTimerParams;

void init_tickers() {
    int i;
    for (i = 0; i < MAX_TICKERS; i++) {
        tickers[i].active = false;
    }

    // Initialize the ticker timer
    Timer_Params_init(&tickerTimerParams);
    tickerTimerParams.periodUnits = Timer_PERIOD_US;
    tickerTimerParams.period = TICKER_TICK_MS * 1000;  // Convert ms to us
    tickerTimerParams.timerMode  = Timer_CONTINUOUS_CALLBACK;
    tickerTimerParams.timerCallback = ticker_timer_callback;

    tickerTimer = Timer_open(CONFIG_GPT_1, &tickerTimerParams); // Ensure CONFIG_GPT_1 is configured

    if (tickerTimer == NULL) {
        System_abort("Failed to initialize ticker timer");
    }

    int32_t status = Timer_start(tickerTimer);
    if (status == Timer_STATUS_ERROR) {
        System_abort("Failed to start ticker timer");
    }
}

// Timer callback function called every 10 ms
void ticker_timer_callback(Timer_Handle myHandle, int_fast16_t status) {
    // Post a semaphore to process tickers in a task context
    Semaphore_post(glo.bios.TickerSem);
}

// Function to process tickers (should be called from a task)
void process_tickers() {
    int i;
    for (i = 0; i < MAX_TICKERS; i++) {
        if (tickers[i].active) {
            if (tickers[i].currentDelay > 0) {
                tickers[i].currentDelay--;
            } else {
                // Add payload for execution
                AddPayload(tickers[i].payload);

                // Handle count and period
                if (tickers[i].count == 0) {
                    // Deactivate the ticker
                    tickers[i].active = false;
                } else {
                    if (tickers[i].count > 0) {
                        tickers[i].count--;
                    }
                    tickers[i].currentDelay = tickers[i].period;
                }
            }
        }
    }
}
