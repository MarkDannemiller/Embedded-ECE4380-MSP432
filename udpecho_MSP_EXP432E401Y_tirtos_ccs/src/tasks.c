/*
 *  ======== tasks.c ========
 */
#include "p100.h"
#include <xdc/runtime/Memory.h>
#include "script.h"
#include "register.h"

#ifdef Globals
extern Globals glo;
#endif

// Process user input to input_buffer -> PayloadQueue
void uart0ReadTask(UArg arg0, UArg arg1) {

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
        // This should not be triggered currently, since uart0ReadTask is the only task that can trigger an emergency stop
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

        handle_UART0();
    }
}

/**
 * @brief Task to write messages from OutMsgQueue to UART
 */
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


/** 
 * Execute Payloads on PayloadQueue.
 * Also handles script execution.
 */
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

        // If script is running, execute the current line and move to the next
        if (glo.scriptPointer >= 0) {
            if (glo.scriptPointer < SCRIPT_LINE_COUNT && scriptLines[glo.scriptPointer]
                && scriptLines[glo.scriptPointer][0] != '\0') { // Check for valid script line. Empty lines are skipped

                int currentLine = glo.scriptPointer;
                execute_payload(scriptLines[glo.scriptPointer]);

                // Increment scriptPointer if the line was executed and did not result in a line change
                if(currentLine == glo.scriptPointer) {
                    glo.scriptPointer++;                    // Move to the next line
                    Semaphore_post(glo.bios.PayloadSem);    // Allow next line to execute
                }
            } else {
                glo.scriptPointer = -1; // End of script
            }
        }

        // Do not execute if PayloadQueue is empty
        if(Queue_empty(glo.bios.PayloadQueue)) {
            continue;
        }

        exec_payload = (PayloadMessage *)Queue_get(glo.bios.PayloadQueue);

        // Execute the payload
        execute_payload(exec_payload->data);

        // if (glo.scriptPointer >= 0) {
        //     glo.scriptPointer++; // Move to the next line
        //     if (glo.scriptPointer < SCRIPT_LINE_COUNT && scriptLines[glo.scriptPointer]) {
        //         AddPayload(scriptLines[glo.scriptPointer]);
        //     } else {
        //         glo.scriptPointer = -1; // End of script
        //     }
        // }

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


void uart1ReadTask(UArg arg0, UArg arg1) {
    while (1) {
        // Check for emergency stop
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

        handle_UART1();
    }
}


void ADCStream() {
    uint16_t *source;
    // A safe buffer size: header + binary data
    // Each sample is 2 bytes * 128 = 256 bytes + overhead
    char longload[512]; 
    int32_t dest_choice;
    int32_t hdrlen;

    while (1) {
        // Wait until ADCBuf has new data
        Semaphore_pend(glo.bios.ADCSemaphore, BIOS_WAIT_FOREVER);

        if (glo.audioController.adcBufControl.RX_Completed == glo.audioController.adcBufControl.RX_Ping) {
            source = glo.audioController.adcBufControl.RX_Completed;
            dest_choice = 0;
            glo.audioController.adcBufControl.ping_count++;
        } else if (glo.audioController.adcBufControl.RX_Completed == glo.audioController.adcBufControl.RX_Pong) {
            source = glo.audioController.adcBufControl.RX_Completed;
            dest_choice = 1;
            glo.audioController.adcBufControl.pong_count++;
        } else {
            AddProgramMessage("Error: ADC RX unknown buffer in ADCStream.\r\n");
            continue;
        }


        // If networking is desired (REG_DIAL1 or REG_DIAL2 != 0), send over network:
        if (glo.audioController.adcBufControl.converting == 2) {
            // If global registers for dial1/dial2 are implemented:
            uint32_t ipDial1 = registers[REG_DIAL1]; // Store the IP address in REG_DIAL1 (R0)
            uint32_t ipDial2 = registers[REG_DIAL2]; // Store the IP address in REG_DIAL2 (R1)

            bool local = true;
            if (ipDial1 != 0) {
                sprintf(longload, "-netudp %d.%d.%d.%d:%d -voice %d 128  ",
                        (uint8_t)(ipDial1 >> 24) & 0xFF,
                        (uint8_t)(ipDial1 >> 16) & 0xFF,
                        (uint8_t)(ipDial1 >> 8 ) & 0xFF,
                        (uint8_t)(ipDial1      ) & 0xFF,
                        DEFAULTPORT, dest_choice);

                hdrlen = (int32_t)(strlen(longload)+1);
                memcpy(&longload[hdrlen], source, sizeof(uint16_t)*DATABLOCKSIZE);
                ParseNetUDP(longload, sizeof(uint16_t)*DATABLOCKSIZE);
                local = false;
            }

            if (ipDial2 != 0) {
                sprintf(longload, "-netudp %d.%d.%d.%d:%d -voice %d 128  ",
                        (uint8_t)(ipDial1 >> 24) & 0xFF,
                        (uint8_t)(ipDial1 >> 16) & 0xFF,
                        (uint8_t)(ipDial1 >> 8 ) & 0xFF,
                        (uint8_t)(ipDial1      ) & 0xFF,
                        DEFAULTPORT, dest_choice+2);
                hdrlen = (int32_t)(strlen(longload)+1);
                memcpy(&longload[hdrlen], source, sizeof(uint16_t)*DATABLOCKSIZE);
                ParseNetUDP(longload, sizeof(uint16_t)*DATABLOCKSIZE);
                local = false;
            }

            if(local == true) {
                // Construct the message: "-voice dest_choice 128  " followed by binary data
                sprintf(longload, "-voice %d 128  ", dest_choice);
                hdrlen = (int32_t)(strlen(longload) + 1);
                memcpy(&longload[hdrlen], source, sizeof(uint16_t) * DATABLOCKSIZE);

                // Now we have a buffer that resembles the format:
                // "-voice <dest_choice> <bufflen>  <binary_data...>"
                // We can call ParseVoice directly:
                ParseVoice(longload);
            }

            // If local == true, we have already done the local voice via ParseVoice above.
            // If not local, we have sent it over network as well.
        }

    }
}


// Code to be implemented
// void ADCStream() {
//     uint16_t *source;
//     char longload[sizeof(uint16_t)*DATABLOCKSIZE+MsgQueueMsgLen];
//     int32_t dest_choice;
//     int32_t hdrlen;
//     bool local = true;

//     while(1){
//         Semaphore_pend(global.Bios.ADCSemaphore, BIOS_WAIT_FOREVER);

//         if(global.ADCBufCtrl.RX_Completed == global.ADCBufCtrl.RX_Ping)
//         {
//             source = global.ADCBufCtrl.RX_Completed;
//             dest_choice = 0;
//             global.ADCBufCtrl.ping_count++;
//         }
//         else if(global.ADCBufCtrl.RX_Completed == global.ADCBufCtrl.RX_Pong)
//         {
//             source = global.ADCBufCtrl.RX_Completed;
//             dest_choice = 1;
//             global.ADCBufCtrl.pong_count++;
//         }
//         else
//         {
//             AddError(STREAM_ERR, "RX_Ping and RX_Pong not completed, lost pointer");
//             return;
//         }
//         //net packaging
//         if(global.Regs.Reg[REG_DIAL1] != 0){
//             sprintf(longload, "-netudp %d.%d.%d.%d:%d -voice %d 128  ",
//                     (uint8_t)(global.Regs.Reg[REG_DIAL1] >> 24) & 0xFF,
//                     (uint8_t)(global.Regs.Reg[REG_DIAL1] >> 16) & 0xFF,
//                     (uint8_t)(global.Regs.Reg[REG_DIAL1] >> 8 ) & 0xFF,
//                     (uint8_t)(global.Regs.Reg[REG_DIAL1]      ) & 0xFF,
//                     DEFAULTPORT, dest_choice);
//             hdrlen = strlen(longload)+1;                                        //Extra byte for null terminator
//             memcpy(&longload[hdrlen], source, sizeof(uint16_t)*DATABLOCKSIZE);
//                                                                                 //Copying adc buff values
//             ParseNetUDP(longload, sizeof(uint16_t)*DATABLOCKSIZE);
//                                                                                 //Add to NETUDP Queue
//             local = false;
//         }

//         if(global.Regs.Reg[REG_DIAL2] != 0){
//             sprintf(longload, "-netudp %d.%d.%d.%d:%d -voice %d 128  ",
//                     (uint8_t)(global.Regs.Reg[REG_DIAL1] >> 24) & 0xFF,
//                     (uint8_t)(global.Regs.Reg[REG_DIAL1] >> 16) & 0xFF,
//                     (uint8_t)(global.Regs.Reg[REG_DIAL1] >> 8 ) & 0xFF,
//                     (uint8_t)(global.Regs.Reg[REG_DIAL1]      ) & 0xFF,
//                     DEFAULTPORT, dest_choice+2);
//             hdrlen = strlen(longload)+1;                                        //Extra byte for null terminator
//             memcpy(&longload[hdrlen], source, sizeof(uint16_t)*DATABLOCKSIZE);
//                                                                                 //Copying adc buff values
//             ParseNetUDP(longload, sizeof(uint16_t)*DATABLOCKSIZE);
//                                                                                 //Add to NETUDP Queue
//             local = false;
//         }

//         if(local == true)
//         {
//             sprintf(longload, "-voice %d 128  ", dest_choice);
//             hdrlen = strlen(longload)+1;
//             memcpy(&longload[hdrlen], source, sizeof(uint16_t)*DATABLOCKSIZE);
//             VoiceParse(longload);
//         }
//     }
// }
