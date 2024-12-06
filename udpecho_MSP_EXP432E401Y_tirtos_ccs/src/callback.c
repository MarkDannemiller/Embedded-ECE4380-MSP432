/*
 *  ======== callback.c ========
 */
#include "p100.h"
#include "callback.h"
#include "audio.h"

CommandCallback callbacks[MAX_CALLBACKS];  // Array of callbacks

void print_all_callbacks() {
    char msg[BUFFER_SIZE];
    int i;
    AddProgramMessage("=========================== Callback Configurations ============================\r\n");
    AddProgramMessage("Index | Count | Payload\r\n");
    AddProgramMessage("------------------------\r\n");
    uint16_t gateKey = GateSwi_enter(gateSwi2);
    for (i = 0; i < MAX_CALLBACKS; i++) {
        sprintf(msg, "%5d | %5d | %s\r\n", i, callbacks[i].count, callbacks[i].payload);
        AddProgramMessage(msg);
    }
    GateSwi_leave(gateSwi2, gateKey);
}

// Timer0 Callback Function
void timer0Callback_fxn(Timer_Handle handle, int_fast16_t status) {
    Swi_post(glo.bios.Timer0_swi);
}

// Timer0 SWI Handler
void timer0SWI(UArg arg0, UArg arg1) {
    // The sine generation logic should be tied to the callback payload now
    // If using callback0 for -sine or -audio, we no longer directly call generateSineSample() here.
    // Instead, -sine will be triggered by execute_payload().

    if (callbacks[0].count != 0 && callbacks[0].payload[0] != '\0') {
        // Immediately execute the callback payload
        execute_payload(callbacks[0].payload);

        uint16_t gateKey = GateSwi_enter(gateSwi2);
        if (callbacks[0].count > 0) {
            callbacks[0].count--;
            if (callbacks[0].count == 0) {
                callbacks[0].payload[0] = '\0';  // Clear payload when count reaches zero
            }
        }
        GateSwi_leave(gateSwi2, gateKey);
    }
}


// SW1 GPIO Callback Function
void sw1Callback_fxn(uint_least8_t index) {
    Swi_post(glo.bios.SW1_swi);
}

// SW1 SWI Handler
void sw1SWI(UArg arg0, UArg arg1) {
    uint16_t gateKey = GateSwi_enter(gateSwi2);
    if (callbacks[1].count != 0) {
        AddPayload(callbacks[1].payload);

        if (callbacks[1].count > 0) {
            callbacks[1].count--;
            if (callbacks[1].count == 0) {
                callbacks[1].payload[0] = '\0';
            }
        }
    }
    GateSwi_leave(gateSwi2, gateKey);
}

// SW2 GPIO Callback Function
void sw2Callback_fxn(uint_least8_t index) {
    Swi_post(glo.bios.SW2_swi);
}

// SW2 SWI Handler
void sw2SWI(UArg arg0, UArg arg1) {
    uint16_t gateKey = GateSwi_enter(gateSwi2);
    if (callbacks[2].count != 0) {
        AddPayload(callbacks[2].payload);

        if (callbacks[2].count > 0) {
            callbacks[2].count--;
            if (callbacks[2].count == 0) {
                callbacks[2].payload[0] = '\0';
            }
        }
    }
    GateSwi_leave(gateSwi2, gateKey);
}

void ADCBufCallback(ADCBuf_Handle handle, ADCBuf_Conversion *conversion, void *buffer, uint32_t channel, int_fast16_t status) {
    // Verify the buffer is either RX_Ping or RX_Pong
    if (buffer != glo.audioController.adcBufControl.RX_Ping && buffer != glo.audioController.adcBufControl.RX_Pong) {
        // If we got some unknown buffer
        // AddError(STREAM_ERR, "Unknown ADC Buffer"); // if AddError is implemented
        while(1);
        AddProgramMessage("Error: Unknown ADC buffer in ADCBufCallback.\r\n");  // TODO implement error
        return;
    }

    // Mark which buffer completed
    glo.audioController.adcBufControl.RX_Completed = buffer;
    // Post semaphore so ADCStream task can handle the data
    Semaphore_post(glo.bios.ADCSemaphore);
}



// Code to be implemented
// void ADCBufCallback(ADCBuf_Handle handle, ADCBuf_Conversion *conversion, void *buffer, uint32_t channel, int_fast16_t status){
//     if(buffer != global.ADCBufCtrl.RX_Ping && buffer != global.ADCBufCtrl.RX_Pong){
//         Swi_post(global.Bios.ADCSWI);
//         return;
//     }
//     global.ADCBufCtrl.RX_Completed = buffer;
//     Semaphore_post(global.Bios.ADCSemaphore);
// }