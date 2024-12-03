/*
 *  ======== callback.c ========
 */
#include "p100.h"
#include "callback.h"

CommandCallback callbacks[MAX_CALLBACKS];  // Array of callbacks

void print_all_callbacks() {
    char msg[BUFFER_SIZE];
    int i;
    AddProgramMessage("=========================== Callback Configurations ============================\r\n");
    AddProgramMessage("Index | Count | Payload\r\n");
    AddProgramMessage("------------------------\r\n");
    for (i = 0; i < MAX_CALLBACKS; i++) {
        sprintf(msg, "%5d | %5d | %s\r\n", i, callbacks[i].count, callbacks[i].payload);
        AddProgramMessage(msg);
    }
}

// Timer0 Callback Function
void timer0Callback_fxn(Timer_Handle handle, int_fast16_t status) {
    Swi_post(glo.bios.Timer0_swi);
}

// Timer0 SWI Handler
void timer0SWI(UArg arg0, UArg arg1) {
    if (callbacks[0].count != 0) {
        AddPayload(callbacks[0].payload);

        if (callbacks[0].count > 0) {
            callbacks[0].count--;
            if (callbacks[0].count == 0) {
                callbacks[0].payload[0] = '\0';  // Clear payload when count reaches zero
            }
        }
    }
}

// SW1 GPIO Callback Function
void sw1Callback_fxn(uint_least8_t index) {
    Swi_post(glo.bios.SW1_swi);
}

// SW1 SWI Handler
void sw1SWI(UArg arg0, UArg arg1) {
    if (callbacks[1].count != 0) {
        AddPayload(callbacks[1].payload);

        if (callbacks[1].count > 0) {
            callbacks[1].count--;
            if (callbacks[1].count == 0) {
                callbacks[1].payload[0] = '\0';
            }
        }
    }
}

// SW2 GPIO Callback Function
void sw2Callback_fxn(uint_least8_t index) {
    Swi_post(glo.bios.SW2_swi);
}

// SW2 SWI Handler
void sw2SWI(UArg arg0, UArg arg1) {
    if (callbacks[2].count != 0) {
        AddPayload(callbacks[2].payload);

        if (callbacks[2].count > 0) {
            callbacks[2].count--;
            if (callbacks[2].count == 0) {
                callbacks[2].payload[0] = '\0';
            }
        }
    }
}
