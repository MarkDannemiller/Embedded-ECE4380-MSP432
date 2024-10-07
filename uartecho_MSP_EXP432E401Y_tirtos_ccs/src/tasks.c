#include "p100.h"
#include <xdc/runtime/Memory.h>

#ifdef Globals
extern Globals glo;
#endif

// Process user input to input_buffer -> PayloadQueue
void uartRead(UArg arg0, UArg arg1) {

    clear_console();

    // Send system info at start of UART
    char* initMsg = formatAboutHeaderMsg();
    char* compileTimeMsg = formatCompileDateTime();

    //UART_write_safe(initMsg, strlen(initMsg));
    //UART_write_safe(compileTimeMsg, strlen(compileTimeMsg));
    AddOutMessage(initMsg);
    AddOutMessage(compileTimeMsg);
    refresh_user_input();

    digitalWrite(0, LOW);

    // Add character to buffer
    // Process other input if necessary
    // Add to payload upon return
    // Tell UARTWrite to update console upon user input

    /* Loop forever handling UART input and queue payloads  */
    while (1) {
        handle_UART();
    }
}

// Write from OutMsgQueue
void uartWrite(UArg arg0, UArg arg1) {
    PayloadMessage *out_message;

    digitalWrite(1, LOW);

    while (1) {
        Semaphore_pend(glo.bios.UARTWriteSem, BIOS_WAIT_FOREVER);  // Wait for object on Queue
        out_message = (PayloadMessage *)Queue_get(glo.bios.OutMsgQueue);
        UART_write_safe_strlen(out_message->data);

        // Free the duplicated string
        Memory_free(NULL, out_message->data, strlen(out_message->data) + 1);
        // Free the message structure
        Memory_free(NULL, out_message, sizeof(PayloadMessage));
    }
}


// Execute Payloads on PayloadQueue
void payloadExecute(UArg arg0, UArg arg1) {
    // Check if PayloadQueue has payload
    // Get payload
    // Execute
    // Add to OutMsgQueue if -print is called
    PMsg exec_payload;

    digitalWrite(2, LOW);

    while (1) {
        Semaphore_pend(glo.bios.PayloadSem, BIOS_WAIT_FOREVER);  // Wait for object on Queue
        exec_payload = (PayloadMessage *)Queue_get(glo.bios.PayloadQueue);
        execute_payload(exec_payload->data);
        // Free the duplicated string
        Memory_free(NULL, exec_payload->data, strlen(exec_payload->data) + 1);
        // Free the message structure
        Memory_free(NULL, exec_payload, sizeof(PayloadMessage));
    }
}
