/*
 * Copyright (c) 2015-2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== p100.c ========
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// Driver Header files
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>

// Driver configuration
#include "ti_drivers_config.h"

// SysBIOS
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/knl/Semaphore.h>

#include <xdc/runtime/Memory.h>
#include <xdc/runtime/System.h>

// System headers
#include "p100.h"
#include "systeminfo.h"
#include "tasks.h"
#include "callback.h"
#include "tickers.h"
#include "register.h"
#include "script.h"


#ifdef Globals
extern Globals glo;
#endif


// Initialize Global Struct from header file
void init_globals() {
    glo.integrityHead = 0x5a5a5a5a;
    glo.integrityTail = 0xa5a5a5a5;
    glo.byteSize = sizeof(glo);

    // UART 0 for console
    glo.cursor_pos = 0;
    glo.progOutputCol = 0;
    glo.progOutputLines = 1;
    int i;
    for(i=3; i<BUFFER_SIZE; i++){
        glo.inputBuffer_uart0[i] = 0;
    }

    // UART 1
    glo.cursor_pos_uart1 = 0;
    for(i=3; i<BUFFER_SIZE; i++){
        glo.inputBuffer_uart1[i] = 0;
    }

    glo.scriptPointer = -1;  // No script loaded

    // Assign the handles to the BiosList struct
    glo.bios.UARTReader0 = UARTReader0;
    glo.bios.UARTWriter = UARTWriter;

    glo.bios.UARTReader1 = UARTReader1;
    
    glo.bios.PayloadExecutor = PayloadExecutor;
    glo.bios.TickerProcessor = TickerProcessor;


    glo.bios.PayloadQueue = PayloadQueue;
    glo.bios.OutMsgQueue = OutMsgQueue;

    glo.bios.UARTWriteSem = UARTWriteSem;
    glo.bios.PayloadSem = PayloadSem;
    glo.bios.TickerSem = TickerSem;

    glo.bios.Timer0_swi = Timer0_swi;
    glo.bios.SW1_swi = SW1_swi;
    glo.bios.SW2_swi = SW2_swi;

    Semaphore_Params semParams;
    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_COUNTING;
    glo.emergencyStopSem = Semaphore_create(0, &semParams, NULL);
}


//================================================
// Error Handling
//================================================


// Define the corresponding error messages
const char* errorMessages[] = {
    "Error: Unknown command. Try -help for list of commands.\r\n",  // ERR_UNKNOWN_COMMAND
    "Error: Buffer Overflow\r\n",                                   // ERR_BUFFER_OF
    "Error: Invalid message size\r\n",                              // ERR_INVALID_MSG_SIZE
    "Error: Null message pointer\r\n",                              // ERR_NULL_MSG_PTR
    "Error: Empty message\r\n",                                     // ERR_EMPTY_MSG
    "Error: Failed to write message\r\n",                           // ERR_MSG_WRITE_FAILED

    "Error: Address out of range\r\n",                              // ERR_ADDR_OUT_OF_RANGE
    "Error: GPIO out of range\r\n",                                 // ERR_GPIO_OUT_OF_RANGE

    "Error: Timer period out of range\r\n",                         // ERR_INVALID_TIMER_PERIOD
    "Error: Timer control function was unsuccessful\r\n",           // ERR_TIMER_STATUS_ERROR
    "Error: UART write was called but did not finish\r\n",          // ERR_UART_COLLISION
    "Error: Invalid callback index.\r\n",                           // ERR_INVALID_CALLBACK_INDEX

    "Error: Missing count parameter.\r\n",                          // ERR_MISSING_COUNT_PARAMETER
    "Error: Missing payload.\r\n",                                  // ERR_MISSING_PAYLOAD
    "Error: Invalid ticker index.\r\n",
    "Error: Missing initial delay parameter.\r\n",
    "Error: Missing period parameter.\r\n",

    "Error: Payload Queue Overflow.\r\n",                           // ERR_PAYLOAD_QUEUE_OF
    "Error: Out Messsage Queue Overflow.\r\n",                      // ERR_OUTMSG_QUEUE_OF

    "Error: Invalid script line number.\r\n",                       // ERR_INVALID_SCRIPT_LINE
    "Error: Missing command to write to script line.\r\n",          // ERR_MISSING_SCRIPT_COMMAND
    "Error: Unknown operation for script command.\r\n"              // ERR_UNKNOWN_SCRIPT_OP
};

const char* errorNames[] = {
    "ERR_UNKNOWN_COMMAND",          // ERR_UNKNOWN_COMMAND
    "ERR_BUFFER_OF",                // ERR_BUFFER_OF
    "ERR_INVALID_MSG_SIZE",         // ERR_INVALID_MSG_SIZE
    "ERR_NULL_MSG_PTR",             // ERR_NULL_MSG_PTR
    "ERR_EMPTY_MSG",                // ERR_EMPTY_MSG
    "ERR_MSG_WRITE_FAILED",         // ERR_MSG_WRITE_FAILED

    "ERR_ADDR_OUT_OF_RANGE",        // ERR_ADDR_OUT_OF_RANGE
    "ERR_GPIO_OUT_OF_RANGE",        // ERR_GPIO_OUT_OF_RANGE

    "ERR_INVALID_TIMER_PERIOD",     // ERR_INVALID_TIMER_PERIOD
    "ERR_TIMER_STATUS_ERROR",       // ERR_TIMER_STATUS_ERROR
    "ERR_UART_COLLISION",           // ERR_UART_COLLISION
    "ERR_INVALID_CALLBACK_INDEX",   // ERR_INVALID_CALLBACK_INDEX

    "ERR_MISSING_COUNT_PARAMETER",  // ERR_MISSING_COUNT_PARAMETER
    "ERR_MISSING_PAYLOAD",          // ERR_MISSING_PAYLOAD
    "ERR_INVALID_TICKER_INDEX",
    "ERR_MISSING_DELAY_PARAMETER",
    "ERR_MISSING_PERIOD_PARAMETER",

    "ERR_PAYLOAD_QUEUE_OF",         // ERR_PAYLOAD_QUEUE_OF
    "ERR_OUTMSG_QUEUE_OF",          // ERR_OUTMSG_QUEUE_OF

    "ERR_INVALID_SCRIPT_LINE",      // ERR_INVALID_SCRIPT_LINE
    "ERR_MISSING_SCRIPT_COMMAND",   // ERR_MISSING_SCRIPT_COMMAND
    "ERR_UNKNOWN_SCRIPT_OP"         // ERR_UNKNOWN_SCRIPT_OPS
};

// Array to store the error counters
int errorCounters[ERROR_COUNT] = {0};

// Function to raise an error and return the corresponding error message
const char* raiseError(Errors error) {
    // Increment the error counter for the given error type
    if (error >= 0 && error < ERROR_COUNT) {
        errorCounters[error]++;
        return errorMessages[error];
    }
    return "Unknown error"; // Fallback for out-of-range error types
}

// Function to convert enum to string
const char* getErrorName(Errors error) {
    if (error >= 0 && error < ERROR_COUNT) {
        return errorNames[error];
    }
    return "Unknown error"; // Fallback for out-of-range values
}


//================================================
// Drivers
//================================================


void init_drivers() {

    /* Call driver init functions */
    GPIO_init();
    UART_init();
    Timer_init();


    // OUTPUTS
    /* Configure the LED 0 pin */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    /* Configure the LED 1 pin */
    GPIO_setConfig(CONFIG_GPIO_LED_1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    /* Configure the LED 2 pin */
    GPIO_setConfig(CONFIG_GPIO_LED_2, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    /* Configure the LED 3 pin */
    GPIO_setConfig(CONFIG_GPIO_LED_3, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    /* Configure the GPIO 4 pin*/
    GPIO_setConfig(CONFIG_GPIO_4, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    /* Configure the GPIO 5 pin*/
    GPIO_setConfig(CONFIG_GPIO_5, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);

    //Configure SW1 (GPIO 6)
    GPIO_setConfig(CONFIG_GPIO_SWITCH_6, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
    GPIO_setCallback(CONFIG_GPIO_SWITCH_6, sw1Callback_fxn);
    GPIO_enableInt(CONFIG_GPIO_SWITCH_6);

    //Configure SW2 (GPIO 7)
    GPIO_setConfig(CONFIG_GPIO_SWITCH_7, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
    GPIO_setCallback(CONFIG_GPIO_SWITCH_7, sw2Callback_fxn);
    GPIO_enableInt(CONFIG_GPIO_SWITCH_7);

    /* Turn on user LED */
    //GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
    digitalWrite(0, HIGH);
    // digitalWrite(1, HIGH);
    // digitalWrite(2, HIGH);
    // digitalWrite(3, HIGH);


    /* Create a UART0 with data processing off. */
    UART_Params_init(&glo.uartParams0);
    glo.uartParams0.writeDataMode = UART_DATA_BINARY;
    glo.uartParams0.readDataMode = UART_DATA_BINARY;
    glo.uartParams0.readReturnMode = UART_RETURN_FULL;
    glo.uartParams0.baudRate = 115200;
    glo.uart0 = UART_open(CONFIG_UART_0, &glo.uartParams0);

    if (glo.uart0 == NULL) {
        // UART_open() failed
        while (1);
    }


    /* Initialize UART1 */
    UART_Params_init(&glo.uartParams1);
    glo.uartParams1.writeDataMode = UART_DATA_BINARY;
    glo.uartParams1.readDataMode = UART_DATA_BINARY;
    glo.uartParams1.readReturnMode = UART_RETURN_FULL;
    glo.uartParams1.baudRate = 115200;
    glo.uart1 = UART_open(CONFIG_UART_1, &glo.uartParams1);

    if (glo.uart1 == NULL) {
        // UART_open() failed
        while (1);
    }


    // Initialize the timer
    Timer_Params_init(&glo.timer0_params);
    glo.timer0_params.periodUnits = Timer_PERIOD_US;
    glo.timer0_params.period = 5000000; // 10 seconds
    glo.timer0_params.timerMode  = Timer_CONTINUOUS_CALLBACK;
    glo.timer0_params.timerCallback = timer0Callback_fxn;
    glo.Timer0 = Timer_open(CONFIG_GPT_0, &glo.timer0_params);
    int32_t status = Timer_start(glo.Timer0);  // Activate Timer0 Periodically

    if (status == Timer_STATUS_ERROR) {
        //Timer_start() failed
        while (1);
    }
}

// Pin mappings - Index pins based on their order for -gpio command
const int pin_count = 8;
const int pinMap[pin_count] = {
    CONFIG_GPIO_LED_0, // -gpio 0
    CONFIG_GPIO_LED_1,
    CONFIG_GPIO_LED_2,
    CONFIG_GPIO_LED_3,
    CONFIG_GPIO_4,
    CONFIG_GPIO_5,
    CONFIG_GPIO_SWITCH_6,
    CONFIG_GPIO_SWITCH_7
};


// Simple language to toggle a pin in the pinMap
void togglePin(int pin_num) {
    GPIO_toggle(pinMap[pin_num]);
}
// Simple language to write HIGH or LOW on a pin in the pinMap
void digitalWrite(int pin_num, pinState state) {
    GPIO_write(pinMap[pin_num], state);
}
// Simple language to convert gpio read to pinState on a pin in the pinMap
pinState digitalRead(int pin_num) {
    return (pinState) GPIO_read(pinMap[pin_num]);
}





/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{

/*
    init_globals();
    init_drivers();

    if (glo.uart0 == NULL) {
        while (1);
    }

    clear_console();

    // Send system info at start of UART
    char* initMsg = formatAboutHeaderMsg();
    char* compileTimeMsg = formatCompileDateTime();

    UART_write_safe(initMsg, strlen(initMsg));
    UART_write_safe(compileTimeMsg, strlen(compileTimeMsg));
    reset_buffer();
    refresh_user_input();*/
    return;
}


/**
 * @brief Emergency stop function to halt all tasks and timers.  Should only be called from uart0ReadTask.
 */
void emergency_stop() {
    int i;

    // Set emergency stop flag
    glo.emergencyStopActive = true;

    // Post to semaphores to unblock tasks
    Semaphore_post(glo.bios.PayloadSem);    // Unblocks executePayloadTask
    Semaphore_post(glo.bios.UARTWriteSem);  // Unblocks uartWriteTask
    Semaphore_post(glo.bios.TickerSem);     // Unblocks tickerProcessingTask
    // For uart0ReadTask, if necessary

    // Wait for all tasks to signal completion
    for (i = 0; i < (NUM_TASKS - 1); i++) {  // -1 for the thread calling emergency_stop, which should only ever be uart0ReadTask
        Semaphore_pend(glo.emergencyStopSem, BIOS_WAIT_FOREVER);
    }

    UART_write_safe_strlen("Emergency stop trigerred.");

    // Disable interrupts
    UInt hwi_key = Hwi_disable();
    UInt swi_key = Swi_disable();

    // Stop the timer
    Timer_stop(glo.Timer0);
    glo.Timer0Period = 0;
    Timer_setPeriod(glo.Timer0, Timer_PERIOD_US, 0);  // Set period to 0 to stop timer

    // Clear the tickers
    for (i = 0; i < MAX_TICKERS; i++) {
        tickers[i].active = false;
        tickers[i].initialDelay = 0;
        tickers[i].period = 0;
        tickers[i].count = 0;
        tickers[i].currentDelay = 0;
        memset(tickers[i].payload, 0, BUFFER_SIZE);
    }

    // Clear the callbacks
    for (i = 0; i < MAX_CALLBACKS; i++) {
        callbacks[i].count = 0;
        memset(callbacks[i].payload, 0, BUFFER_SIZE);
    }

    // Clear the payload queue
    while (!Queue_empty(glo.bios.PayloadQueue)) {
        Queue_Elem *elem = Queue_get(glo.bios.PayloadQueue);
        PayloadMessage *message = (PayloadMessage *)elem;
        Memory_free(NULL, message->data, strlen(message->data) + 1);
        Memory_free(NULL, message, sizeof(PayloadMessage));
    }
    Semaphore_reset(glo.bios.PayloadSem, 0);  // Reset the semaphore count (no payloads to execute)
    glo.scriptPointer = -1;  // Reset the script pointer to disable script execution

    // Clear the UART write queue
    while (!Queue_empty(glo.bios.OutMsgQueue)) {
        Queue_Elem *elem = Queue_get(glo.bios.OutMsgQueue);
        PayloadMessage *message = (PayloadMessage *)elem;
        Memory_free(NULL, message->data, strlen(message->data) + 1);
        Memory_free(NULL, message, sizeof(PayloadMessage));
    }
    Semaphore_reset(glo.bios.UARTWriteSem, 0);  // Reset the semaphore count (no messages to write)

    // Re-enable interrupts
    Hwi_restore(hwi_key);
    Swi_restore(swi_key);

    // Clear the emergency stop flag
    glo.emergencyStopActive = false;

    // Optionally, post to semaphores to wake tasks up
    // Semaphore_post(glo.bios.PayloadSem);
    // Semaphore_post(glo.bios.UARTWriteSem);
    // Semaphore_post(glo.bios.TickerSem);

    // You can also add a message indicating the system is resuming normal operation
    AddProgramMessage("Emergency stop completed.\r\n");
    AddProgramMessage("Timer, callbacks, tickers, and payloads have been cleared.\r\n");
    AddProgramMessage("Resuming normal operation.\r\n");
    Task_sleep(100);  // Sleep for a short time to let system become stable.
    // TODO remove need for Task_sleep() -> right now it is locking up the system if not included and the user presses '`' continuously
}


//================================================
// Utility
//================================================

// Custom string duplication function using Memory_alloc
char *memory_strdup(const char *src) {
    if (src == NULL) {
        return NULL;
    }

    size_t len = strlen(src) + 1; // +1 for the null terminator
    char *dst = (char *)Memory_alloc(NULL, len, 0, NULL);
    if (dst != NULL) {
        strcpy(dst, src);
    }
    return dst;
}

int isPrintable(char ch) { return (ch >= 32 && ch <= 126); }  // ASCII printable characters


//================================================
// UART Writer
//================================================

/*void AddOutMessage(char *data) {
    PayloadMessage message;
    message.data = data;
    Queue_put(glo.bios.OutMsgQueue, &(message.elem));  // Add payload to Queue
    //Semaphore_post((Semaphore_Object *) glo.BiosList.PayloadSem);  // Post to Semaphore ready to execute
    Semaphore_post(glo.bios.UARTWriteSem);  // Post to Semaphore ready to execute
}*/

void AddProgramMessage(char *data) {

    if(Semaphore_getCount(glo.bios.UARTWriteSem) >= MAX_QUEUE_SIZE) {
        raiseError(ERR_PAYLOAD_QUEUE_OF); // Can't display error if queue is full!
        return;  // Do not add message in case of overflow
    }

    PayloadMessage *message = (PayloadMessage *)Memory_alloc(NULL, sizeof(PayloadMessage), 0, NULL);
    if (message == NULL) {
        System_abort("Failed to allocate memory for message");
    }

    message->data = memory_strdup(data);
    message->isProgramOutput = true;

    if (message->data == NULL) {
        Memory_free(NULL, message, sizeof(PayloadMessage));
        System_abort("Failed to allocate memory for message data");
    }

    Queue_put(glo.bios.OutMsgQueue, &(message->elem));
    Semaphore_post(glo.bios.UARTWriteSem);
}


void AddOutMessage(const char *data) {
    if(Semaphore_getCount(glo.bios.UARTWriteSem) >= MAX_QUEUE_SIZE) {
        raiseError(ERR_PAYLOAD_QUEUE_OF); // Can't display error if queue is full!
        return;  // Do not add message in case of overflow
    }

    PayloadMessage *message = (PayloadMessage *)Memory_alloc(NULL, sizeof(PayloadMessage), 0, NULL);
    if (message == NULL) {
        System_abort("Failed to allocate memory for message");
    }

    message->data = memory_strdup(data);
    message->isProgramOutput = false;

    if (message->data == NULL) {
        Memory_free(NULL, message, sizeof(PayloadMessage));
        System_abort("Failed to allocate memory for message data");
    }

    Queue_put(glo.bios.OutMsgQueue, &(message->elem));
    Semaphore_post(glo.bios.UARTWriteSem);
}


bool UART_write_safe(const char *message, int size) {
    // Check if UART handle is valid (assuming glo.uart0 should be valid)
    if (glo.uart0 == NULL) {
        return false;  // If UART is not initialized, we can't send any error messages
    }

    // Return false and send error message if size is 0 or negative
    if (size <= 0) {
        UART_write_safe_strlen(raiseError(ERR_INVALID_MSG_SIZE));
        //UART_write(glo.uart0, "Error: Invalid message size.\r\n", 28);
        return false;
    }

    // Return false and send error message if message is a null pointer
    if (message == NULL) {
        UART_write_safe_strlen(raiseError(ERR_NULL_MSG_PTR));
        //UART_write(glo.uart0, "Error: Null message pointer.\r\n", 29);
        return false;
    }

    // Return false and send error message if message is empty (first character is the null terminator)
    if (message[0] == '\0') {
        UART_write_safe_strlen(raiseError(ERR_EMPTY_MSG));
        //UART_write(glo.uart0, "Error: Empty message.\r\n", 22);
        return false;
    }

    // Optional: Return false and send error message if size exceeds the maximum buffer size
    if (size > MAX_MESSAGE_SIZE) {
        UART_write_safe_strlen(raiseError(ERR_BUFFER_OF));
       // UART_write(glo.uart0, "Error: Message size exceeds maximum size.\r\n", 42);
        return false;
    }

    // Write the message and check if the write operation is successful
    if (UART_write(glo.uart0, message, size) == UART_STATUS_ERROR) {
        raiseError(ERR_MSG_WRITE_FAILED);
        return false;
    }
    return true;  // Return true if all checks pass and write is successful
}

// Writes without needing the size.  Uses strlen() to get size
bool UART_write_safe_strlen(const char *message) {
    int size = strlen(message);
    return UART_write_safe(message, size);
}




//================================================
// UART Reader
//================================================


void handle_UART0() {
    char key_in;

    // Catch cursor out of bounds
    if(glo.cursor_pos < 0) {
        glo.cursor_pos = 0;
    }

    UART_read(glo.uart0, &key_in, 1);

    if(key_in == '`') {
        emergency_stop();  // Emergency stop with special character
        return;
    }
    else if (key_in == '\r' || key_in == '\n') {  // Enter key
        if(glo.cursor_pos > 0) {
            // Process the completed input
            AddOutMessage("\r\n");  // Use AddOutMessage to move cursor to new line
            AddPayload(glo.inputBuffer_uart0);
        }
        reset_buffer();
        return;
    } else if (key_in == '\b' || key_in == 0x7F) {  // Backspace key
        if (glo.cursor_pos > 0) {
            // Remove the last character
            glo.inputBuffer_uart0[--glo.cursor_pos] = '\0';
        }
        refresh_user_input();  // TODO TEST
    } else if (key_in == '\033') {  // Escape character for arrow keys
        // Implement handling of arrow keys if needed
        // Normal Operation
    } else {
        // Add the character to the buffer if it's not full
        if (glo.cursor_pos < BUFFER_SIZE - 1) {
            glo.inputBuffer_uart0[glo.cursor_pos++] = key_in;
            glo.inputBuffer_uart0[glo.cursor_pos] = '\0';
            AddOutMessage(&key_in);  // Use AddOutMessage to echo the character
        } else {
            AddOutMessage("\r\n");  // Move to a new line
            reset_buffer();
            AddProgramMessage(raiseError(ERR_BUFFER_OF));  // Use AddProgramMessage for error
        }
    }

    //glo.input_msg_size = glo.cursor_pos;
    //refresh_user_input();  // This will use AddOutMessage()
}


void reset_buffer() {
    glo.cursor_pos = 0;
    memset(glo.inputBuffer_uart0, 0, BUFFER_SIZE);
    AddProgramMessage(NEW_LINE_RETURN);  // Use AddOutMessage to move to a new line
}


// Clears the entire console window
void clear_console() {
    //UART_write_safe(CLEAR_CONSOLE, strlen(CLEAR_CONSOLE));
    AddOutMessage(CLEAR_CONSOLE);
}


void refresh_user_input() {
    AddOutMessage(CLEAR_LINE_RESET);  // Clear the input line
    AddOutMessage("> ");              // Display the prompt
    if(glo.cursor_pos > 0)
        AddOutMessage(glo.inputBuffer_uart0); // Display the current input buffer
}


//=============================================================================
// UART Console Functions (Called by UARTWriter)
//=============================================================================

void moveCursorUp(int lines) {
    if (lines <= 0) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "\033[%dA", lines);
    UART_write_safe(buf, strlen(buf));
}

void moveCursorDown(int lines) {
    if (lines <= 0) return;
    // char buf[16];
    // snprintf(buf, sizeof(buf), "\033[%dB", lines);
    // UART_write_safe(buf, strlen(buf));
    int i;
    for(i = 0; i < lines; i++) {
        UART_write_safe("\r\n", 2);
    }
}

void moveCursorToColumn(int col) {
    char buf[16];
    snprintf(buf, sizeof(buf), "\033[%dG", col + 1);  // Columns start at 1
    UART_write_safe(buf, strlen(buf));
}

void refreshUserInputLine() {
    // Move cursor to the beginning of the line
    UART_write_safe("\r", 1);

    // Clear the line
    UART_write_safe("\033[K", 3);  // Clear from cursor to end of line

    // Write the input prompt and current buffer
    UART_write_safe("> ", 2);
    if(glo.cursor_pos > 0)
        UART_write_safe(glo.inputBuffer_uart0, glo.cursor_pos);
}


//================================================
// UART1 Comms
//================================================

void handle_UART1() {
    char key_in;

    // Catch cursor out of bounds
    if(glo.cursor_pos_uart1 < 0) {
        glo.cursor_pos_uart1 = 0;
    }

    UART_read(glo.uart1, &key_in, 1);

    if (key_in == '\r' || key_in == '\n') {  // Enter key
        if(glo.cursor_pos_uart1 > 0) {
            // Process the completed input
            AddPayload(glo.inputBuffer_uart1);

            // Echo the input to the console
            AddProgramMessage("Receveived Over UART1: ");
            AddProgramMessage(glo.inputBuffer_uart1);
        }
        reset_buffer_uart1();
    } else if (key_in == '\b' || key_in == 0x7F) {  // Backspace key
        if (glo.cursor_pos_uart1 > 0) {
            // Remove the last character
            glo.inputBuffer_uart1[--glo.cursor_pos_uart1] = '\0';
        }
    } else {
        // Add the character to the buffer if it's not full
        if (glo.cursor_pos_uart1 < BUFFER_SIZE - 1) {
            glo.inputBuffer_uart1[glo.cursor_pos_uart1++] = key_in;
            glo.inputBuffer_uart1[glo.cursor_pos_uart1] = '\0';
        } else {
            // Buffer overflow handling
            reset_buffer_uart1();
            // Optionally, you can send an error message or handle overflow
        }
    }
}

void reset_buffer_uart1() {
    // Reset the cursor position for UART1 input buffer
    glo.cursor_pos_uart1 = 0;

    // Clear the input buffer for UART1
    memset(glo.inputBuffer_uart1, 0, BUFFER_SIZE);
}


//================================================
// Payload Management
//================================================

// Add a payload string to the payload queue
/*void AddPayload(char *payload) {
    PayloadMessage message;
    message.data = payload;
    Queue_put(glo.bios.PayloadQueue, &(message.elem));  // Add payload to Queue
    //Semaphore_post((Semaphore_Object *) glo.BiosList.PayloadSem);  // Post to Semaphore ready to execute
    Semaphore_post(glo.bios.PayloadSem);  // Post to Semaphore ready to execute
}*/
void AddPayload(char *payload) {

    if(Semaphore_getCount(glo.bios.PayloadSem) >= MAX_QUEUE_SIZE) {
        AddProgramMessage(raiseError(ERR_PAYLOAD_QUEUE_OF));
        return;  // Do not add payload in case of overflow
    }

    // Allocate memory for the message structure
    PayloadMessage *message = (PayloadMessage *)Memory_alloc(NULL, sizeof(PayloadMessage), 0, NULL);
    if (message == NULL) {
        System_abort("Failed to allocate memory for payload message");
    }

    // Duplicate the payload string using Memory_alloc
    message->data = memory_strdup(payload);
    if (message->data == NULL) {
        Memory_free(NULL, message, sizeof(PayloadMessage));
        System_abort("Failed to allocate memory for payload data");
    }

    // Add the message to the queue
    Queue_put(glo.bios.PayloadQueue, &(message->elem));
    Semaphore_post(glo.bios.PayloadSem);
}



// Executes the passed payload command
void execute_payload(char *msg) {
    // Copy the original message buffer to preserve its contents
    char msgBufferCopy[BUFFER_SIZE];
    strncpy(msgBufferCopy, msg, BUFFER_SIZE);

    // Start tokenizing the input message
    char *token = strtok(msgBufferCopy, " \t\r\n");
    char *p;  // Declare p at the beginning of the block

    // Convert token to lowercase for case-insensitive comparison
    for (p = token; *p; ++p) {
        *p = tolower(*p);
    }

    //=========================================================================
    // Check for commmand
    if (strcmp(token,           "-about") == 0) {
        CMD_about();
    }
    else if (strcmp(token,      "-callback") == 0) {
        CMD_callback();
    }
    else if (strcmp(token,      "-error") == 0) {
        CMD_error();
    }
    else if (strcmp(token,      "-gpio") == 0) {
        CMD_gpio();
    }
    else if (strcmp(token,      "-help") == 0) {
        CMD_help();
    }
    else if (strcmp(token,      "-if") == 0) {
        CMD_if();
    }
    else if (strcmp(token,      "-memr") == 0) {
        CMD_memr();
    }
    else if (strcmp(token,      "-print") == 0) {
        CMD_print();
    }
    else if (strcmp(token,      "-reg") == 0) {
        CMD_reg();
    }
    else if (strcmp(token,      "-rem") == 0) {
        CMD_rem();
    }
    else if (strcmp(token,      "-script") == 0) {
        CMD_script();
    }
    else if (strcmp(token,      "-ticker") == 0) {
        CMD_ticker();
    }
    else if (strcmp(token,      "-timer") == 0) {
        CMD_timer();
    }
    if (strcmp(token,           "-uart") == 0) {
        CMD_uart();
    }
    else {
        AddProgramMessage(raiseError(ERR_UNKNOWN_COMMAND));

    }
    //=========================================================================
}


//================================================
// Commands
//================================================

// Function to print about information
void CMD_about() {
    static char aboutMessage[MAX_MESSAGE_SIZE];  // Adjust size as needed
    sprintf(aboutMessage,
            "==================================== About =====================================\r\n"
            "Student Name: Mark Dannemiller\r\n"
            "Assignment: %s\r\n"
            "Version: v%d.%d\r\n"
            "Compiled on: %s\r\n",
            ASSIGNMENT, VERSION, SUBVERSION, __DATE__ " " __TIME__);

    //UART_write_safe(aboutMessage, strlen(aboutMessage));
    AddProgramMessage(aboutMessage);
}

void CMD_callback() {
    // Parse index
    char *index_token = strtok(NULL, " \t\r\n");

    if (!index_token) {
        // No index provided, display all callbacks
        print_all_callbacks();
        return;
    }

    int index = atoi(index_token);
    if (index < 0 || index >= MAX_CALLBACKS) {
        AddProgramMessage(raiseError(ERR_INVALID_CALLBACK_INDEX));
        return;
    }

    // Parse count
    char *count_token = strtok(NULL, " \t\r\n");
    int count = -1;
    if (count_token) {
        count = atoi(count_token);
    } else {
        //AddProgramMessage(raiseError(ERR_MISSING_COUNT_PARAMETER));
        AddProgramMessage("Clearing callback.\r\n");
        callbacks[index].count = 0;
        memset(callbacks[index].payload, 0, BUFFER_SIZE);
        return;
    }

    // The rest is the payload
    char *payload_start = strtok(NULL, "\r\n");
    if (!payload_start) {
        AddProgramMessage(raiseError(ERR_MISSING_PAYLOAD));
        return;
    }

    // Copy the payload
    strncpy(callbacks[index].payload, payload_start, BUFFER_SIZE);
    callbacks[index].payload[BUFFER_SIZE - 1] = '\0';  // Ensure null-terminated

    // Set the count
    callbacks[index].count = count;

    // Acknowledge
    char msg[BUFFER_SIZE];
    sprintf(msg, "Callback %d set with count %d and payload: %s\r\n", index, count, callbacks[index].payload);
    AddProgramMessage(msg);
}

void CMD_error() {
    char str[BUFFER_SIZE];
    int i;

    //UART_write_safe_strlen("Error Count:\r\n");
    AddProgramMessage("Error Count:\r\n");
    for(i = 0; i < ERROR_COUNT; i++) {
        sprintf(str, "|  %s: %d\r\n", getErrorName(i), errorCounters[i]);
        //UART_write_safe_strlen(str);
        AddProgramMessage(str);
    }
}

void CMD_gpio() {

    // Next parameter should be the pin number
    char *pin_token = strtok(NULL, " \t\r\n");
    char *p; // For lowercase operation
    char *pin_ptr;
    uint32_t pin_num;
    char output_msg[BUFFER_SIZE];

    if(!pin_token) {
        AddProgramMessage("===================================== GPIO =====================================\r\n");
        // If no pin number is provided, read all pins
        for (pin_num = 0; pin_num < pin_count; pin_num++) {
            pinState read_state = digitalRead(pin_num);
            sprintf(output_msg, "Read => Pin: %d  State: %d\r\n", pin_num, read_state);
            AddProgramMessage(output_msg);
        }
        return;
    } else {
        pin_num = strtoul(pin_token, &pin_ptr, 10);
    }

    // Catch out of bounds case
    if(pin_num >= pin_count) {
        AddProgramMessage(raiseError(ERR_GPIO_OUT_OF_RANGE));
        return;
    }

    // Next parameter should be the pin operation
    char *op_token = strtok(NULL, " \t\r\n");

    // Convert token to lowercase for case-insensitive comparison
    for (p = op_token; *p; ++p) {
        *p = tolower(*p);
    }

    // "w" = write and needs to collect the state
    if(strcmp(op_token, "w") == 0) {
        char *state_token = strtok(NULL, " \t\r\n");
        // Convert token to lowercase for case-insensitive comparison
        for (p = state_token; *p; ++p) {
            *p = tolower(*p);
        }

        // Determine if writing high or low (default low)
        if(strcmp(state_token, "1") == 0 || strcmp(state_token, "high") == 0 || strcmp(state_token, "true") == 0) {
            sprintf(output_msg, "Wrote => Pin: %d  State: 1\r\n", pin_num);
            digitalWrite(pin_num, HIGH);
        }
        else {
            sprintf(output_msg, "Wrote => Pin: %d  State: 0\r\n", pin_num);
            digitalWrite(pin_num, LOW);
        }
    }
    // Toggle pin operation
    else if(strcmp(op_token, "t") == 0) {
        togglePin(pin_num);
        pinState read_state = digitalRead(pin_num);
        sprintf(output_msg, "Toggled => Pin: %d State: %d\r\n", pin_num, read_state);
    }
    else {  // Otherwise assume read
        pinState read_state = digitalRead(pin_num);
        sprintf(output_msg, "Read => Pin: %d  State: %d\r\n", pin_num, read_state);
        AddProgramMessage(output_msg);
        return;
    }

    // AddProgramMessage(output_msg);  // Removed printout because we can visually see pin changes
}

// Function to print help information
void CMD_help() {
    const char *helpMessage;
    // If second token exists, then check for a custom help message
    char *cmd_arg_token = strtok(NULL, " \t\r\n");

    const char *help_prefix = "===================================== Help =====================================\r\n";

    // Check for commmand
    if (strcmp(cmd_arg_token,           "about") == 0 || strcmp(cmd_arg_token,           "-about") == 0) {
        helpMessage =
           //================================================================================ <-80 characters
            "Command: -about\n\r"
            "| args: none\n\r"
            "| Description: Displays system information.\r\n"
            "| Example usage: \"-about\" -> Displays info about the system.\r\n";

    }
    else if (strcmp(cmd_arg_token,      "callback") == 0 || strcmp(cmd_arg_token,        "-callback") == 0) {
        helpMessage =
           //================================================================================ <-80 characters
            "Command: -callback [index] [count] [payload]\n\r"
            "| args:\n\r"
            "| | index: Callback index (0 for timer, 1 for SW1, 2 for SW2).\r\n"
            "| | count: Number of times to execute the payload (-1 for infinite).\r\n"
            "| | payload: Command to execute when the event occurs.\r\n"
            "| Description: Configures a callback to execute a payload when an event occurs.\r\n"
            "|              If no arguments are provided, displays all callbacks.\r\n"
            "| Example usage: \"-callback 1 2 -gpio 3 t\" -> On SW1 press, toggle GPIO 3 twice.\r\n"
            "| Example usage: \"-callback\" -> Displays all callbacks, counts, and  payloads.\r\n";

    }
    else if (strcmp(cmd_arg_token,      "error") == 0 || strcmp(cmd_arg_token,           "-error") == 0) {
        helpMessage =
           //================================================================================ <-80 characters
            "Command: -error\n\r"
            "| args: none\n\r"
            "| Description: Displays number of times that each error type has triggered.\r\n"
            "| Example usage: \"-error\" -> Displays error count by type.\r\n";

    }
    else if (strcmp(cmd_arg_token,      "gpio") == 0  || strcmp(cmd_arg_token,           "-gpio") == 0) {
        helpMessage =
           //================================================================================ <-80 characters
            "Command: -gpio [pin][function][val]\n\r"
            "| args:\n\r"
            "| | pin: GPIO Pin within range [0,7].\r\n"
            "| | function: [r]ead [w]rite [t]oggle operation on pin.\r\n"
            "| | val: Used only in write mode 1=HIGH 0=LOW.\r\n"
            "| Description: Performs pin function on selected GPIO.\r\n"
            "| Example usage: \"-gpio 1 w 1\" -> Writes HIGH on GPIO 1\r\n";
            "| Example usage: \"-gpio 6 r\" -> Reads input on GPIO 6\r\n";
            "| Example usage: \"-gpio 2 t\" -> Toggles output of GPIO 2\r\n";

    }
    else if (strcmp(cmd_arg_token,      "help") == 0  || strcmp(cmd_arg_token,           "-help") == 0) {
        helpMessage =
           //================================================================================ <-80 characters
            "Command: -help [command]\n\r"
            "| args:\n\r"
            "| | command: One of the acceptable command entries. Default none displays all\r\n"
            "| |          available commands to the console.\r\n"
            "| Description: Displays helpful information about one or all commands.\r\n"
            "| Example usage: \"-help\" -> Displays all available commands.\r\n"
            "| Example usage: \"-help print\" -> Displays information about print command.\r\n";

    }
    else if(strcmp(cmd_arg_token,       "if") == 0  || strcmp(cmd_arg_token,              "-if") == 0) {
        helpMessage =
           //================================================================================ <-80 characters
            "Command: -if (A COND B) ? DESTT : DESTF\n\r"
            "| args:\n\r"
            "| | A: The first operand, which can be a register or an immediate value\r\n"
            "| |    (#value)\r\n"
            "| | COND: The condition to evaluate. Acceptable values are >, =, or <.\r\n"
            "| | B: The second operand, which can be a register or an immediate value\r\n"
            "| |    (#value).\r\n"
            "| | DESTT: The payload to execute if the condition is true. Can be null.\r\n"
            "| | DESTF: The payload to execute if the condition is false. Can be null.\r\n"
            "| Description: Executes a payload based on the condition.\r\n"
            "| Example usage: \"-if (10 > #0) ? -script 5 : -print FALSE\" -> Executes script\r\n"
            "|                 line 5 if the contents of register 10 are greater than 0,\r\n"
            "|                 otherwise prints FALSE.\r\n";

    }
    else if (strcmp(cmd_arg_token,      "memr") == 0  || strcmp(cmd_arg_token,           "-memr") == 0) {
        helpMessage =
           //================================================================================ <-80 characters
            "Command: -memr [addresss]\n\r"
            "| args:\n\r"
            "| | address: The hexadecimal address to display the contents of. Acceptable\r\n"
            "| |          ranges are from (0x00->0xFFFFF) and (0x20000000->0x2003FFFF)\r\n"
            "| Description: Displays the contents of the inputted memory address.\r\n"
            "| Example usage: \"-memr 1000\" -> Displays the contents of address 0x1000.\r\n";

    }
    else if (strcmp(cmd_arg_token,      "print") == 0  || strcmp(cmd_arg_token,          "-print") == 0) {
        helpMessage =
           //================================================================================ <-80 characters
            "Command: -print [message]\n\r"
            "| args:\n\r"
            "| | messsage: The contents to print. Displays the entire line after \"-print \".\r\n"
            "| Description: Displays the message inputted after command statement.\r\n"
            "| Example usage: \"-print abc\" -> Displays the contents of address 0x1000.\r\n";

    }
    else if (strcmp(cmd_arg_token, "reg") == 0 || strcmp(cmd_arg_token, "-reg") == 0) {
        helpMessage =
           //================================================================================ <-80 characters
            "Command: -reg [operation] [operands]\n\r"
            "| Operations:\n\r"
            "| | mov dest src        : Move src to dest.\r\n"
            "| | xchg reg1 reg2      : Exchange values of reg1 and reg2.\r\n"
            "| | inc reg             : Increment reg by 1.\r\n"
            "| | dec reg             : Decrement reg by 1.\r\n"
            "| | add dest src        : Add src to dest.\r\n"
            "| | sub dest src        : Subtract src from dest.\r\n"
            "| | neg reg             : Negate reg.\r\n"
            "| | and dest src        : Bitwise AND of dest and src.\r\n"
            "| | ior dest src        : Bitwise OR of dest and src.\r\n"
            "| | xor dest src        : Bitwise XOR of dest and src.\r\n"
            "| | mul dest src        : Multiply dest by src.\r\n"
            "| | div dest src        : Divide dest by src.\r\n"
            "| | rem dest src        : Remainder of dest divided by src.\r\n"
            "| | max dest src        : Set dest to max of dest and src.\r\n"
            "| | min dest src        : Set dest to min of dest and src.\r\n"
            "| Operands:\r\n"
            "| | Registers: r0 to r31, (0 to 31 also works)\r\n"
            "| | Immediate: #value or #xvalue\r\n"
            "| | Memory Address: @address\r\n"
            "| Examples:\r\n"
            "| | -reg mov r0 #10     : Set R0 to 10.\r\n"
            "| | -reg add r0 r1      : Add R1 to R0.\r\n"
            "| | -reg inc r0         : Increment R0.\r\n";
    }
    else if (strcmp(cmd_arg_token, "rem") == 0 || strcmp(cmd_arg_token, "-rem") == 0) {
        helpMessage =
           //================================================================================ <-80 characters
            "Command: -rem [remark]\n\r"
            "| args:\n\r"
            "| | remark: Any text to be ignored by the interpreter.\r\n"
            "| Description: Used for adding comments or remarks in scripts.\r\n"
            "| Examples:\r\n"
            "| | -rem This is a comment line.\r\n";
    }
    else if (strcmp(cmd_arg_token,      "script") == 0 || strcmp(cmd_arg_token,         "-script") == 0) {
        helpMessage =
           //================================================================================ <-80 characters
            "Command: -script [line_number] [operation] [command]\n\r"
            "| args:\n\r"
            "| | line_number: Script line number (0 to 63).\r\n"
            "| | operation:\r\n"
            "| | | w [command]: Write a command to the script line.\r\n"
            "| | | x: Execute the script starting from the line.\r\n"
            "| | | c: Clear the script line.\r\n"
            "| Description: Manages and executes scripts of commands.\r\n"
            "| Examples:\r\n"
            "| | -script                : Display all script lines.\r\n"
            "| | -script 3              : Display script line 3.\r\n"
            "| | -script 17 w -gpio 0 t : Write '-gpio 0 t' to script line 17.\r\n"
            "| | -script 17 x           : Execute script starting from line 17.\r\n"
            "| | -script 17 c           : Clear script line 17.\r\n"
            "| Special Case:\r\n"
            "| | -script r              : Reset the script pointer to -1 (stops execution).\r\n";
    }
    else if (strcmp(cmd_arg_token,      "timer") == 0 || strcmp(cmd_arg_token,           "-timer") == 0) {
        helpMessage =
           //================================================================================ <-80 characters
            "Command: -timer [period_us]\n\r"
            "| args:\n\r"
            "| | period_us: The period of Timer0 in microseconds. Minimum value is 100 us\r\n"
            "| Description: Sets the period of Timer0 to the specified period in microseconds.\r\n"
            "| |            If no period is specified, displays the current Timer0 period.\r\n"
            "| Example usage: \"-timer 100000\" -> Sets Timer0 period to 100,000 us (100 ms).\r\n"
            "| Example usage: \"-timer\" -> Displays the current Timer0 period.\r\n"
            "| Example usage: \"-timer 0\" -> Disables the timer\r\n";

    }
    else if (strcmp(cmd_arg_token,      "ticker") == 0 || strcmp(cmd_arg_token,           "-ticker") == 0) {
        helpMessage =
           //================================================================================ <-80 characters
            "Command: -ticker [index] [initialDelay] [period] [count] [payload]\n\r"
            "| args:\n\r"
            "| | index: Ticker index (0 to 15).\r\n"
            "| | initialDelay: Initial delay before first execution in 10 ms units.\r\n"
            "| | period: Period between executions in 10 ms units.\r\n"
            "| | count: Number of times to repeat (-1 for infinite).\r\n"
            "| | payload: Command to execute when the ticker triggers.\r\n"
            "| Description: Configures a ticker to execute a payload after a delay and\r\n"
            "| |            repeat it. If no arguments are given, displays all tickers.\r\n"
            "| Example usage: \"-ticker 3 100 100 5 -gpio 2 t\" -> Uses ticker 3, waits 1000\r\n"
            "| |              ms, then toggles GPIO 2 every 1000 ms, repeating 5 times.\r\n"
            "| Example usage: \"-ticker\" -> Displays all tickers and payloads.\r\n";

    }
    else if (strcmp(cmd_arg_token,      "uart" == 0) || strcmp(cmd_arg_token,           "-uart") == 0) {
        helpMessage =
           //================================================================================ <-80 characters
            "Command: -uart [payload]\r\n"
            "| args:\r\n"
            "| | payload: The message to send over UART1.\r\n"
            "| Description: Sends a payload message over UART1.\r\n"
            "| Example usage: \"-uart -print Hello, World!\" -> Sends -print over UART1.\r\n";
    }
    else {
        helpMessage =
           //================================================================================ <-80 characters
            "| Supported Command [arg1][arg2]...   |  Command Description\r\n"
            "|-------------------------------------|-----------------------------------------\r\n"
            "| -about                              |  Display program information.\r\n"
            "| -callback  [index] [count] [payload]|  Configures a callback to execute a\r\n"
            "|                                     |  payload when an event occurs.\r\n"
            "| -error                              |  Displays number of times that each\r\n"
            "|                                     |  error type has triggered.\r\n"
            "| -gpio      [pin] [function] [val]   |  Performs pin function on selected\r\n"
            "|                                     |  GPIO.\r\n"
            "| -help      [command]                |  Display this help message or a\r\n"
            "|                                     |  specific message based on command\r\n"
            "| -if        [A] [COND] [B] ?         |  Executes a payload based on the\r\n"
            "|            [DESTT] : [DESTF]        |  condition.\r\n" 
            "|                                     |  to a command. I.E \"-help print\"\r\n"
            "| -memr      [address]                |  Display contents of given memory\r\n"
            "|                                     |  address.\r\n"
            "| -print     [string]                 |  Display inputted string.\r\n"
            "|                                     |  I.E \"-print abc\"\r\n"
            "| -reg       [operation] [operands]   |  Perform operation on specified\r\n"
            "|                                     |  register.\r\n"
            "| -rem       [remark]                 |  Add comments or remarks in scripts.\r\n" 
            "| -script    [line_number] [operation]|  Manage and execute scripts of\r\n"
            "|                                     |  commands.\r\n"
            "| -ticker    [index] [initialDelay]   |  Configures a ticker to execute a\r\n"
            "|            [period] [count] [payload]  payload after a delay and repeat it.\r\n"
            "| -timer     [period_us]              |  Sets the period of Timer0 in\r\n"
            "|                                        microseconds.\r\n"
            "| -uart      [payload]                |  Sends a payload message over UART1.\r\n";
    }

    //UART_write_safe(helpMessage, strlen(helpMessage));`
    AddProgramMessage(help_prefix);
    AddProgramMessage(helpMessage);
}

void CMD_if() {
    // Parse the condition (A COND B) or A COND B, where COND is >, =, or <
    char *condition_part = strtok(NULL, "?");
    if (!condition_part) {
        AddProgramMessage("Error: Invalid syntax for -if command.\r\n");  // TODO: Add to errors
        return;
    }

    // Trim leading and trailing whitespaces
    trim(condition_part);

    // Remove surrounding parentheses if present
    if (condition_part[0] == '(' && condition_part[strlen(condition_part) - 1] == ')') {
        condition_part[strlen(condition_part) - 1] = '\0'; // Remove closing parenthesis
        condition_part++; // Move pointer past opening parenthesis
    }

    // Extract operands and condition
    char operandA[BUFFER_SIZE], operandB[BUFFER_SIZE], cond[3];
    if (sscanf(condition_part, "%s %2[><=] %s", operandA, cond, operandB) != 3) {
        AddProgramMessage("Error: Invalid condition format.\r\n");
        return;
    }

    // Parse DESTT and DESTF
    char *dest_part = strtok(NULL, "");
    if (!dest_part) {
        AddProgramMessage("Error: Missing destinations in -if command.\r\n");  // TODO: Add to errors
        return;
    }

    char *destT = strtok(dest_part, ":");
    char *destF = strtok(NULL, "");

    // Trim leading and trailing whitespaces
    trim(destT);
    if (destF) trim(destF);

    // Evaluate the condition
    int valueA = getOperandValue(operandA);
    int valueB = getOperandValue(operandB);
    if (valueA == INT64_MIN || valueB == INT64_MIN) {
        // Error message already handled in getOperandValue
        return;
    }

    // Determine the result of the condition
    bool conditionResult = false;
    if (strcmp(cond, ">") == 0) {
        conditionResult = (valueA > valueB);
    } else if (strcmp(cond, "=") == 0 || strcmp(cond, "==") == 0) {
        conditionResult = (valueA == valueB);
    } else if (strcmp(cond, "<") == 0) {
        conditionResult = (valueA < valueB);
    } else {
        AddProgramMessage("Error: Invalid condition operator.\r\n");  // TODO: Add to errors
        return;
    }

    // Execute the appropriate destination
    if (conditionResult) {
        if (destT && strlen(destT) > 0) {
            execute_destination(destT);
        }
    } else {
        if (destF && strlen(destF) > 0) {
            execute_destination(destF);
        }
    }
}



// Prints the contents of a given memory address
void CMD_memr() {

    int32_t val;
    char msg[BUFFER_SIZE];
    uint32_t memaddr;
    uint32_t memorig;

    // Get the next token in args (should be a hex value)
    char *addr_token = strtok(NULL, " \t\r\n");
    char *addr_ptr;

    if(!addr_token)
        memorig = 0;
    else
        memorig = strtoul(addr_token, &addr_ptr, 16);

    // Getting memaddr of the "region/row" that memorig lies within
    memaddr = 0xFFFFFFF0 & memorig;                     // Mask 15 bits to 0 (base 16), with eventual intent to print 16 bytes
    // TODO replace with a check for valid memory address range using is_valid_memory_address() in register.c
    if(memaddr >= 0x100000 && memaddr < 0x20000000)     // Too high for FLASH too low for SRAM
        goto ERROR38;
    if(memaddr >= 0x20040000)  // Too high for SRAM
        goto ERROR38;

    // Header and Surrounding Addresses
    sprintf(msg, "MEMR\r\n");
    //UART_write_safe(msg, strlen(msg));
    AddProgramMessage(msg);
    sprintf(msg, "%#010x %#010x %#010x %#010x\r\n", memaddr+0xC, memaddr+0x8, memaddr+0x4, memaddr);
    //UART_write_safe(msg, strlen(msg));
    AddProgramMessage(msg);

    // Content of Surrounding Addresses
    val = *(int32_t *) (memaddr + 0xC);
    sprintf(msg, "%#010x ", val);
    //UART_write_safe(msg, strlen(msg));
    AddProgramMessage(msg);
    val = *(int32_t *) (memaddr + 0x8);
    sprintf(msg, "%#010x ", val);
    //UART_write_safe(msg, strlen(msg));
    AddProgramMessage(msg);
    val = *(int32_t *) (memaddr + 0x4);
    sprintf(msg, "%#010x ", val);
    //UART_write_safe(msg, strlen(msg));
    AddProgramMessage(msg);
    val = *(int32_t *) (memaddr + 0x0);
    sprintf(msg, "%#010x\r\n", val);
    //UART_write_safe(msg, strlen(msg));
    AddProgramMessage(msg);

    // Display last byte (8 bits) of memorig address
    uint8_t last_byte = *(uint8_t *) memorig;
    sprintf(msg, "%#04x\n\r", last_byte);  // Display the last byte in hexadecimal
    //UART_write_safe(msg, strlen(msg));
    AddProgramMessage(msg);
    return;

ERROR38:
    //UART_write_safe_strlen(raiseError(ERR_ADDR_OUT_OF_RANGE));
    AddProgramMessage(raiseError(ERR_ADDR_OUT_OF_RANGE));
}

/// @brief Prints the message after the first token, which should be "-print"
void CMD_print() {
    char *msg_token = strtok(NULL, "\r\n");

    if(msg_token) {
        //UART_write_safe(msg_token, strlen(msg_token));
        AddProgramMessage(msg_token);
        AddProgramMessage("\r\n");
    }
}

/// @brief Function to perform operation of specified register
void CMD_reg() {
    // Get the operation token
    char *op_token = strtok(NULL, " \t\r\n");

    // If no operation token is provided, print all registers
    if (!op_token) {
        print_all_registers();
        return;
    }

    // Convert operation token to lowercase for case-insensitive comparison
    char *p;
    for (p = op_token; *p; ++p) {
        *p = tolower(*p);
    }

    // Initialize argument tokens
    char *arg1_token = strtok(NULL, " \t\r\n");
    char *arg2_token = strtok(NULL, " \t\r\n");

    // Handle operations that require one operand
    if (strcmp(op_token, "inc") == 0 ||
        strcmp(op_token, "dec") == 0 ||
        strcmp(op_token, "neg") == 0 ||
        strcmp(op_token, "not") == 0) {

        if (!arg1_token) {
            AddProgramMessage("Error: Missing operand.\r\n");
            return;
        }

        if (strcmp(op_token, "inc") == 0) {
            reg_inc(arg1_token);
        } else if (strcmp(op_token, "dec") == 0) {
            reg_dec(arg1_token);
        } else if (strcmp(op_token, "neg") == 0) {
            reg_neg(arg1_token);
        } else if (strcmp(op_token, "not") == 0) {
            reg_not(arg1_token);
        }

    }
    // Handle operations that require two operands
    else if (strcmp(op_token, "mov") == 0 ||
             strcmp(op_token, "xchg") == 0 ||
             strcmp(op_token, "add") == 0 ||
             strcmp(op_token, "sub") == 0 ||
             strcmp(op_token, "and") == 0 ||
             strcmp(op_token, "ior") == 0 ||
             strcmp(op_token, "xor") == 0 ||
             strcmp(op_token, "mul") == 0 ||
             strcmp(op_token, "div") == 0 ||
             strcmp(op_token, "rem") == 0 ||
             strcmp(op_token, "max") == 0 ||
             strcmp(op_token, "min") == 0) {

        if (!arg1_token || !arg2_token) {
            AddProgramMessage("Error: Missing operands.\r\n");
            return;
        }

        if (strcmp(op_token, "mov") == 0) {
            reg_mov(arg1_token, arg2_token);
        } else if (strcmp(op_token, "xchg") == 0) {
            reg_xchg(arg1_token, arg2_token);
        } else if (strcmp(op_token, "add") == 0) {
            reg_add(arg1_token, arg2_token);
        } else if (strcmp(op_token, "sub") == 0) {
            reg_sub(arg1_token, arg2_token);
        } else if (strcmp(op_token, "and") == 0) {
            reg_and(arg1_token, arg2_token);
        } else if (strcmp(op_token, "ior") == 0) {
            reg_ior(arg1_token, arg2_token);
        } else if (strcmp(op_token, "xor") == 0) {
            reg_xor(arg1_token, arg2_token);
        } else if (strcmp(op_token, "mul") == 0) {
            reg_mul(arg1_token, arg2_token);
        } else if (strcmp(op_token, "div") == 0) {
            reg_div(arg1_token, arg2_token);
        } else if (strcmp(op_token, "rem") == 0) {
            reg_rem(arg1_token, arg2_token);
        } else if (strcmp(op_token, "max") == 0) {
            reg_max(arg1_token, arg2_token);
        } else if (strcmp(op_token, "min") == 0) {
            reg_min(arg1_token, arg2_token);
        }

    }
    // Unknown operation
    else {
        AddProgramMessage("Error: Unknown operation.\r\n");
    }
}

void CMD_rem() {
    // Do nothing or provide acknowledgment
    // For now, we can do nothing
}

void CMD_script() {
    char *arg1 = strtok(NULL, " \t\r\n");       // Line number or NULL
    char *arg2 = strtok(NULL, " \t\r\n");       // 'w', 'x', 'c', or NULL
    char *rest_of_line = strtok(NULL, "\r\n");  // The rest of the command

    if (!arg1) {
        // No arguments: Display all script lines
        print_all_script_lines();
        return;
    }

    // -script r: Reset the script execution
    if (strcmp(arg1, "r") == 0) {
        // Reset the script execution
        glo.scriptPointer = -1;  // Reset script pointer
        return;
    }

    int line_number = atoi(arg1);
    if (line_number < 0 || line_number >= SCRIPT_LINE_COUNT) {
        AddProgramMessage(raiseError(ERR_INVALID_SCRIPT_LINE));
        return;
    }

    if (!arg2) {
        // Single argument: Display the specified script line
        print_script_line(line_number);
        return;
    }

    if (strcmp(arg2, "w") == 0) {
        // Write command to script line
        if (!rest_of_line) {
            AddProgramMessage(raiseError(ERR_MISSING_SCRIPT_COMMAND));
            return;
        }
        strncpy(scriptLines[line_number], rest_of_line, SCRIPT_LINE_SIZE - 1);
        scriptLines[line_number][SCRIPT_LINE_SIZE - 1] = '\0';  // Ensure null-terminated
        char msg[SCRIPT_LINE_SIZE];
        sprintf(msg, "Script line %d set to: %s\r\n", line_number, scriptLines[line_number]);
        AddProgramMessage(msg);

    } else if (strcmp(arg2, "x") == 0) {
        // Execute script starting from line_number
        execute_script_from_line(line_number);

    } else if (strcmp(arg2, "c") == 0) {
        // Clear script line
        scriptLines[line_number][0] = '\0';
        char msg[BUFFER_SIZE];
        sprintf(msg, "Script line %d cleared.\r\n", line_number);
        AddProgramMessage(msg);
    } else {
        AddProgramMessage(raiseError(ERR_UNKNOWN_SCRIPT_OP));
    }
}


/// @brief Command function to parse and set up a ticker
void CMD_ticker() {
    // Parse index
    char *index_token = strtok(NULL, " \t\r\n");

    if (!index_token) {
        // No index provided, display all tickers
        print_all_tickers();
        return;
    }

    int index = atoi(index_token);
    if (index < 0 || index >= MAX_TICKERS) {
        AddProgramMessage(raiseError(ERR_INVALID_TICKER_INDEX));
        return;
    }

    // Parse initial delay
    char *delay_token = strtok(NULL, " \t\r\n");
    uint32_t initialDelay = 0;
    if (delay_token) {
        initialDelay = strtoul(delay_token, NULL, 10);
    } else {
        AddProgramMessage(raiseError(ERR_MISSING_DELAY_PARAMETER));
        return;
    }

    // Parse period
    char *period_token = strtok(NULL, " \t\r\n");
    uint32_t period = 0;
    if (period_token) {
        period = strtoul(period_token, NULL, 10);
    } else {
        AddProgramMessage(raiseError(ERR_MISSING_PERIOD_PARAMETER));
        return;
    }

    // Parse count
    char *count_token = strtok(NULL, " \t\r\n");
    int32_t count = -1;
    if (count_token) {
        count = atoi(count_token);
    } else {
        AddProgramMessage(raiseError(ERR_MISSING_COUNT_PARAMETER));
        return;
    }

    // The rest is the payload
    char *payload_start = strtok(NULL, "\r\n");
    if (!payload_start) {
        AddProgramMessage(raiseError(ERR_MISSING_PAYLOAD));
        return;
    }

    // Configure the ticker
    tickers[index].active = true;
    tickers[index].initialDelay = initialDelay;
    tickers[index].period = period;
    tickers[index].count = count;
    tickers[index].currentDelay = initialDelay;
    strncpy(tickers[index].payload, payload_start, BUFFER_SIZE);
    tickers[index].payload[BUFFER_SIZE - 1] = '\0';  // Ensure null-terminated

    // Acknowledge
    char msg[BUFFER_SIZE];
    sprintf(msg, "Ticker %d set with delay %u, period %u, count %d, payload: %s\r\n",
            index, initialDelay, period, count, tickers[index].payload);
    AddProgramMessage(msg);
}

/// @brief Command function to parse and set up a timer
void CMD_timer() {

    // Next parameter should be the period in microseconds
    char *val_token = strtok(NULL, " \t\r\n");
    char output_msg[BUFFER_SIZE];

    if(!val_token) {
        // No period provided, display current Timer0 period
        sprintf(output_msg, "Current Timer0 period is %u us\r\n", glo.Timer0Period);
        AddProgramMessage(output_msg);
        return;
    }

    char *val_ptr;
    uint32_t val_us = strtoul(val_token, &val_ptr, 10);

    // Stop the timer if the value is 0
    if(val_us == 0) {
        // Stop the timer
        Timer_stop(glo.Timer0);
        glo.Timer0Period = 0;
        Timer_setPeriod(glo.Timer0, Timer_PERIOD_US, 0);  // Set period to 0 to stop timer
        AddProgramMessage("Timer0 stopped\r\n");
        return;
    }

    // Throw error if value is too low
    if(val_us < MIN_TIMER_PERIOD_US) {
        //UART_write_safe_strlen(raiseError(ERR_INVALID_TIMER_PERIOD));
        AddProgramMessage(raiseError(ERR_INVALID_TIMER_PERIOD));
        return; // No change
    }
    //UART_write_safe_strlen("Time to write...\r\n");
    AddProgramMessage("Time to write...\r\n");

    Timer_stop(glo.Timer0);
    glo.Timer0Period = val_us;
    Timer_setPeriod(glo.Timer0, Timer_PERIOD_US, val_us);
    int32_t status = Timer_start(glo.Timer0);

    if(status == Timer_STATUS_ERROR) {
        //UART_write_safe_strlen(raiseError(ERR_TIMER_STATUS_ERROR));
        AddProgramMessage(raiseError(ERR_TIMER_STATUS_ERROR));
    }
    else {
        sprintf(output_msg, "Set Timer0 period to %d us\r\n", val_us);
        //UART_write_safe_strlen(output_msg);
        AddProgramMessage(output_msg);
    }
}

/**
 * @brief Command function to send a payload over UART 1
 */
void CMD_uart() {
    // Get the rest of the line as the payload
    char *payload = strtok(NULL, "\r\n");
    if (!payload) {
        AddProgramMessage("Error: No payload provided for -uart command.\r\n");  // TODO: Add to errors
        return;
    }

    // Send the payload out UART 1
    int len = strlen(payload);
    if (UART_write(glo.uart1, payload, len) != len) {
        AddProgramMessage("Error: UART 1 write failed.\r\n");  // TODO: Add to errors
        return;
    }

    // Send a newline to complete the command
    UART_write(glo.uart1, "\r\n", 2);

    // Acknowledge the command
    AddProgramMessage("Payload sent over UART 1.\r\n");
}
