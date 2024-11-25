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
    AddProgramMessage(initMsg);
    AddProgramMessage(compileTimeMsg);
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

    while (1) {
        Semaphore_pend(glo.bios.UARTWriteSem, BIOS_WAIT_FOREVER);
        out_message = (PayloadMessage *)Queue_get(glo.bios.OutMsgQueue);

        if (out_message->isProgramOutput) {
            // Move cursor up to program output line
            //moveCursorUp(progOutputLines);
            moveCursorUp(1);

            // Move cursor to the last known program output column position
            moveCursorToColumn(glo.progOutputCol);

            // Process the message character by character
            char *data = out_message->data;
            int len = strlen(data);

            int i;
            for (i = 0; i < len; i++) {
                char ch = data[i];

                if (ch == '\n') {
                    // Newline or carriage return resets the column position
                    // Move to the beginning of next line
                    UART_write_safe("\r\n", 2);
                    glo.progOutputCol = 0;
                    // Need to clear the line which had the user input before
                    UART_write_safe_strlen(CLEAR_LINE_RESET);
                } else if (isPrintable(ch) && ch != '\r') {
                    // Printable character
                    UART_write_safe(&ch, 1);
                    glo.progOutputCol++;

                    if (glo.progOutputCol > MAX_LINE_LENGTH) {
                        // Move to the beginning of next line
                        UART_write_safe("\r\n", 2);
                        glo.progOutputCol = 0;
                    }
                }
                // Handle other characters if needed
            }

            // After writing, move cursor down to user input line
            //moveCursorDown(progOutputLines);
            moveCursorDown(1);

            // Refresh user input line
            refreshUserInputLine();
        } else {
            // Handle user input updates
            UART_write_safe_strlen(out_message->data);
        }

        // Free allocated memory
        Memory_free(NULL, out_message->data, strlen(out_message->data) + 1);
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

// Ticker processing task
void tickerProcessingTask(UArg arg0, UArg arg1) {
    while (1) {
        // Wait for the semaphore to be posted by the ticker timer
        Semaphore_pend(glo.bios.TickerSem, BIOS_WAIT_FOREVER);

        // Process tickers
        process_tickers();
    }
}
