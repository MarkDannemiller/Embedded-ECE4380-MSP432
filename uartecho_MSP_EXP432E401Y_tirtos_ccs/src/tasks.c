#include "p100.h"
#include <xdc/runtime/Memory.h>
#include "script.h"

#ifdef Globals
extern Globals glo;
#endif

// Process user input to input_buffer -> PayloadQueue
void uartReadTask(UArg arg0, UArg arg1) {

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
        // Check for emergency stop
        // This should not be triggered currently, since uartReadTask is the only task that can trigger an emergency stop
        if (glo.emergencyStopActive) {
            // Perform cleanup if necessary
            // ...

            // Signal completion
            Semaphore_post(glo.emergencyStopSem);

            // Wait until emergency stop is cleared
            while (glo.emergencyStopActive) {
                Task_sleep(100);  // Sleep to yield CPU
            }

            // Continue or re-initialize as necessary
            continue;
        }

        handle_UART();
    }
}

// Write from OutMsgQueue
void uartWriteTask(UArg arg0, UArg arg1) {
    PayloadMessage *out_message;

    while (1) {
        Semaphore_pend(glo.bios.UARTWriteSem, BIOS_WAIT_FOREVER);

        // Check for emergency stop
        if (glo.emergencyStopActive) {
            // Perform cleanup
            // No need to clear OutMsgQueue; emergency_stop will handle it

            // Signal completion
            Semaphore_post(glo.emergencyStopSem);

            // Wait until emergency stop is cleared
            while (glo.emergencyStopActive) {
                Task_sleep(100);
            }
            continue;
        }

        // Do not write if OutMsgQueue is empty
        if(Queue_empty(glo.bios.OutMsgQueue)) {
            continue;
        }

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
void executePayloadTask(UArg arg0, UArg arg1) {
    // Check if PayloadQueue has payload
    // Get payload
    // Execute
    // Add to OutMsgQueue if -print is called
    PMsg exec_payload;

    while (1) {
        Semaphore_pend(glo.bios.PayloadSem, BIOS_WAIT_FOREVER);  // Wait for payload

        // Check for emergency stop
        if (glo.emergencyStopActive) {
            // Perform cleanup
            // No need to clear PayloadQueue; emergency_stop will handle it

            // Signal completion
            Semaphore_post(glo.emergencyStopSem);

            // Wait until emergency stop is cleared
            while (glo.emergencyStopActive) {
                Task_sleep(100);
            }
            continue;
        }

        // Do not execute if PayloadQueue is empty
        if(Queue_empty(glo.bios.PayloadQueue)) {
            continue;
        }

        exec_payload = (PayloadMessage *)Queue_get(glo.bios.PayloadQueue);

        // Execute the payload
        execute_payload(exec_payload->data);

        // If part of a script, queue the next line
        if (glo.scriptPointer >= 0) {
            glo.scriptPointer++; // Move to the next line
            if (glo.scriptPointer < SCRIPT_LINE_COUNT && scriptLines[glo.scriptPointer]) {
                AddPayload(scriptLines[glo.scriptPointer]);
            } else {
                glo.scriptPointer = -1; // End of script
            }
        }

        // Free allocated memory
        Memory_free(NULL, exec_payload->data, strlen(exec_payload->data) + 1);
        Memory_free(NULL, exec_payload, sizeof(PayloadMessage));
    }
}

// Ticker processing task
void tickerProcessingTask(UArg arg0, UArg arg1) {
    while (1) {
        // Wait for the semaphore to be posted by the ticker timer
        Semaphore_pend(glo.bios.TickerSem, BIOS_WAIT_FOREVER);

        // Check for emergency stop
        if (glo.emergencyStopActive) {
            // Perform cleanup if necessary
            // ...

            // Signal completion
            Semaphore_post(glo.emergencyStopSem);

            // Wait until emergency stop is cleared
            while (glo.emergencyStopActive) {
                Task_sleep(100);
            }
            continue;
        }

        // Process tickers
        process_tickers();
    }
}
