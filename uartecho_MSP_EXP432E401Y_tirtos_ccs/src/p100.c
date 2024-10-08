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
 *  ======== uartecho.c ========
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
#include <ti/sysbios/hal/Hwi.h>

#include <xdc/runtime/Memory.h>
#include <xdc/runtime/System.h>

// System headers
#include "p100.h"
#include "systeminfo.h"
#include "tasks.h"
#include "callback.h"
#include "tickers.h"


#ifdef Globals
extern Globals glo;
#endif


// Initialize Global Struct from header file
void init_globals() {
    glo.integrityHead = 0x5a5a5a5a;
    glo.integrityTail = 0xa5a5a5a5;
    glo.byteSize = sizeof(glo);
    glo.cursor_pos = 0;
    glo.msg_size = 0;
    int i;
    for(i=3; i<BUFFER_SIZE; i++){
            glo.msgBuffer[i] = 0;
    }

    // Assign the handles to the BiosList struct
    glo.bios.UARTReader = UARTReader;
    glo.bios.UARTWriter = UARTWriter;
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
}


//================================================
// Error Handling
//================================================


// Define the corresponding error messages
const char* errorMessages[] = {
    "Error: Unknown command. Try -help for list of commands.\r\n", // ERR_UNKNOWN_COMMAND
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
    "Error: Out Messsage Queue Overflow.\r\n"                       // ERR_OUTMSG_QUEUE_OF
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
    "ERR_OUTMSG_QUEUE_OF"           // ERR_OUTMSG_QUEUE_OF
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
    digitalWrite(1, HIGH);
    digitalWrite(2, HIGH);
    digitalWrite(3, HIGH);

    /* Create a UART with data processing off. */
    UART_Params_init(&glo.uartParams);
    glo.uartParams.writeDataMode = UART_DATA_BINARY;
    glo.uartParams.readDataMode = UART_DATA_BINARY;
    glo.uartParams.readReturnMode = UART_RETURN_FULL;
    glo.uartParams.baudRate = 115200;

    glo.uart = UART_open(CONFIG_UART_0, &glo.uartParams);


    Timer_Params_init(&glo.timer0_params);
    glo.timer0_params.periodUnits = Timer_PERIOD_US;
    glo.timer0_params.period = 5000000; // 10 seconds
    glo.timer0_params.timerMode  = Timer_CONTINUOUS_CALLBACK;
    glo.timer0_params.timerCallback = timer0Callback_fxn;
    glo.Timer0 = Timer_open(CONFIG_GPT_0, &glo.timer0_params);

    // Activate Timer0 Periodically
    int32_t status = Timer_start(glo.Timer0);
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

    if (glo.uart == NULL) {
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
}





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

void AddOutMessage(char *data) {

    if(Semaphore_getCount(glo.bios.UARTWriteSem) >= MAX_QUEUE_SIZE) {
        raiseError(ERR_PAYLOAD_QUEUE_OF); // Can't display error if queue is full!
        return;  // Do not add message in case of overflow
    }

    PayloadMessage *message = (PayloadMessage *)Memory_alloc(NULL, sizeof(PayloadMessage), 0, NULL);
    if (message == NULL) {
        System_abort("Failed to allocate memory for message");
    }

    message->data = memory_strdup(data);
    if (message->data == NULL) {
        Memory_free(NULL, message, sizeof(PayloadMessage));
        System_abort("Failed to allocate memory for message data");
    }

    Queue_put(glo.bios.OutMsgQueue, &(message->elem));
    Semaphore_post(glo.bios.UARTWriteSem);
}


//TODO implement "write_line" with a maximum line size of 60, and a maximum total message size
bool UART_write_safe(const char *message, int size) {
    // Check if UART handle is valid (assuming glo.uart should be valid)
    if (glo.uart == NULL) {
        return false;  // If UART is not initialized, we can't send any error messages
    }

    // Return false and send error message if size is 0 or negative
    if (size <= 0) {
        UART_write_safe_strlen(raiseError(ERR_INVALID_MSG_SIZE));
        //UART_write(glo.uart, "Error: Invalid message size.\r\n", 28);
        return false;
    }

    // Return false and send error message if message is a null pointer
    if (message == NULL) {
        UART_write_safe_strlen(raiseError(ERR_NULL_MSG_PTR));
        //UART_write(glo.uart, "Error: Null message pointer.\r\n", 29);
        return false;
    }

    // Return false and send error message if message is empty (first character is the null terminator)
    if (message[0] == '\0') {
        UART_write_safe_strlen(raiseError(ERR_EMPTY_MSG));
        //UART_write(glo.uart, "Error: Empty message.\r\n", 22);
        return false;
    }

    // TODO Check each line of message (split by \n\r) for buffer overflow
    // Optional: Return false and send error message if size exceeds the maximum buffer size
    if (size > MAX_MESSAGE_SIZE) {
        UART_write_safe_strlen(raiseError(ERR_BUFFER_OF));
       // UART_write(glo.uart, "Error: Message size exceeds maximum size.\r\n", 42);
        return false;
    }

    // Write the message and check if the write operation is successful
    // TODO handle collision when UART_write is not finished
    if (UART_write(glo.uart, message, size) == UART_STATUS_ERROR) {
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


void handle_UART() {
    char key_in;

    // Catch cursor out of bounds
    if(glo.cursor_pos < 0) {
        glo.cursor_pos = 0;
    }

    UART_read(glo.uart, &key_in, 1);
    glo.msgBuffer[glo.cursor_pos] = key_in;


    if (key_in == '\r' || key_in == '\n') {  // Enter key

        //TODO see if this is best for handling no command
        if(glo.cursor_pos > 0) {
            // Process the completed input
            //UART_write_safe("\r\n", 2);  // Move to a new line to display output
            AddOutMessage("\r\n");
            //execute_payload(glo.msgBuffer);
            AddPayload(glo.msgBuffer);
        }
        reset_buffer();
        return;
    } else if (key_in == '\b' || key_in == 0x7F) {  // Backspace key
        if (glo.cursor_pos > 0) {
            // Remove the last character
            glo.msgBuffer[--glo.cursor_pos] = '\0';
        }
    } else if (key_in == '\033') {  // Escape character for arrow keys
        // Implement handling of arrow keys if needed
    // Normal Operation
    } else {
        // First, if this is start of message, get a new line
        if(glo.msg_size == -1) {
            AddOutMessage("\r\n");
        }

        // Add the character to the buffer if it's not full
        if (glo.cursor_pos < BUFFER_SIZE - 1) {
            glo.msgBuffer[glo.cursor_pos++] = key_in;
            glo.msgBuffer[glo.cursor_pos] = '\0';
        }
        else {
            //UART_write_safe("\n\r", 2);  // Move to a new line to display output
            AddOutMessage("\r\n");
            AddOutMessage(raiseError(ERR_BUFFER_OF));
            //UART_write_safe_strlen(raiseError(ERR_BUFFER_OF));
            //UART_write_safe("Error: Buffer Overflow.\r\n", 25);  // Move to a new line to display output
            reset_buffer();
        }
    }

    glo.msg_size = glo.cursor_pos;//= glo.cursor_pos > glo.msg_size ? glo.cursor_pos : glo.msg_size;
    refresh_user_input();

    // Increment cursor only if in bounds of buffer
}

void reset_buffer() {

    glo.cursor_pos = 0;
    glo.msg_size = -1;
    int i;
    //TODO dont do this -> clear buffer in better way?
    for(i=3; i<BUFFER_SIZE; i++){
        glo.msgBuffer[i] = 0;
    }

    //UART_write(glo.uart, NEW_LINE_RETURN, sizeof(NEW_LINE_RETURN) - 1);
    AddOutMessage(NEW_LINE_RETURN);
}


void clear_console() {
    //UART_write_safe(CLEAR_CONSOLE, strlen(CLEAR_CONSOLE));
    AddOutMessage(CLEAR_CONSOLE);
}


void refresh_user_input() {
    //UART_write_safe(CLEAR_LINE_RESET, sizeof(CLEAR_LINE_RESET) - 1);
    //UART_write_safe(glo.msgBuffer, glo.msg_size+1);

    AddOutMessage(CLEAR_LINE_RESET);
    AddOutMessage("> ");
    if(glo.msg_size > 0)
        AddOutMessage(glo.msgBuffer);
}




//================================================
// Payload Management
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
        AddOutMessage(raiseError(ERR_PAYLOAD_QUEUE_OF));
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

    bool commandRecognized = false;  // To track if any valid command is recognized

    //token = strtok(NULL, " \t\r\n");

    // Convert token to lowercase for case-insensitive comparison
    for (p = token; *p; ++p) {
        *p = tolower(*p);
    }

    //=========================================================================
    // Check for commmand
    if (strcmp(token,           "-about") == 0) {
        CMD_about();
        commandRecognized = true;
    }
    else if (strcmp(token,      "-callback") == 0) {
        CMD_callback();
        commandRecognized = true;
    }
    else if (strcmp(token,      "-error") == 0) {
        CMD_error();
        commandRecognized = true;
    }
    else if (strcmp(token,      "-gpio") == 0) {
        CMD_gpio();
        commandRecognized = true;
    }
    else if (strcmp(token,      "-help") == 0) {
        CMD_help();
        commandRecognized = true;
    }
    else if (strcmp(token,      "-memr") == 0) {
        CMD_memr();
        commandRecognized = true;
    }
    else if (strcmp(token,      "-print") == 0) {
        CMD_print();
        commandRecognized = true;
    }
    else if (strcmp(token,      "-ticker") == 0) {
        CMD_ticker();
        commandRecognized = true;
    }
    else if (strcmp(token,      "-timer") == 0) {
        CMD_timer();
        commandRecognized = true;
    }

    //=========================================================================

    if (!commandRecognized) {  // Only send error if no valid command has been found yet
        //UART_write_safe_strlen(raiseError(ERR_UNKNOWN_COMMAND));
        AddOutMessage(raiseError(ERR_UNKNOWN_COMMAND));
        //UART_write_safe("Error: Unknown command. Try -help for list of commands.\n\r", strlen("Error: Unknown command. Try -help for list of commands.\r\n"));
    }
}


//================================================
// Commands
//================================================

// Function to print about information
void CMD_about() {
    static char aboutMessage[MAX_MESSAGE_SIZE];  // Adjust size as needed
    sprintf(aboutMessage,
            "Student Name: Mark Dannemiller\r\n"
            "Assignment: %s\r\n"
            "Version: v%d.%d\r\n"
            "Compiled on: %s\r\n",
             ASSIGNMENT, VERSION, SUBVERSION, __DATE__ " " __TIME__);

    //UART_write_safe(aboutMessage, strlen(aboutMessage));
    AddOutMessage(aboutMessage);
}

void CMD_callback() {
    // Parse index
    char *index_token = strtok(NULL, " \t\r\n");
    int index = -1;
    if (index_token) {
        index = atoi(index_token);
    }
    if (index < 0 || index >= MAX_CALLBACKS) {
        AddOutMessage(raiseError(ERR_INVALID_CALLBACK_INDEX));
        return;
    }

    // Parse count
    char *count_token = strtok(NULL, " \t\r\n");
    int count = -1;
    if (count_token) {
        count = atoi(count_token);
    } else {
        AddOutMessage(raiseError(ERR_MISSING_COUNT_PARAMETER));
        return;
    }

    // The rest is the payload
    char *payload_start = strtok(NULL, "\r\n");
    if (!payload_start) {
        AddOutMessage(raiseError(ERR_MISSING_PAYLOAD));
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
    AddOutMessage(msg);
}

void CMD_error() {
    char str[BUFFER_SIZE];
    int i;

    //UART_write_safe_strlen("Error Count:\r\n");
    AddOutMessage("Error Count:\r\n");
    for(i = 0; i < ERROR_COUNT; i++) {
        sprintf(str, "|  %s: %d\r\n", getErrorName(i), errorCounters[i]);
        //UART_write_safe_strlen(str);
        AddOutMessage(str);
    }
}

void CMD_gpio() {

    // Next parameter should be the pin number
    char *pin_token = strtok(NULL, " \t\r\n");
    char *p; // For lowercase operation
    char *pin_ptr;
    uint32_t pin_num;
    char output_msg[BUFFER_SIZE];

    if(!pin_token)
            pin_num = 0;
        else
            pin_num = strtoul(pin_token, &pin_ptr, 10);

    // Catch out of bounds case
    if(pin_num >= pin_count) {
        //UART_write_safe_strlen(raiseError(ERR_GPIO_OUT_OF_RANGE));
        AddOutMessage(raiseError(ERR_GPIO_OUT_OF_RANGE));
        return;
    }

    // Next parameter should be the pin operation
    char *op_token = strtok(NULL, " \t\r\n");

    // Convert token to lowercase for case-insensitive comparison
    for (p = op_token; *p; ++p) {
        *p = tolower(*p);
    }

    // "w" = write and needs to collect the state
    if(strcmp(op_token,         "w") == 0) {
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
    else if(strcmp(op_token,    "t") == 0) {
        togglePin(pin_num);
        pinState read_state = digitalRead(pin_num);
        sprintf(output_msg, "Toggled => Pin: %d State: %d\r\n", pin_num, read_state);
    }
    else {  // Otherwise assume read
        pinState read_state = digitalRead(pin_num);
        sprintf(output_msg, "Read => Pin: %d  State: %d\r\n", pin_num, read_state);
    }

    //UART_write_safe_strlen(output_msg);
    AddOutMessage(output_msg);
}

// Function to print help information
void CMD_help() {
    const char *helpMessage;
    // If second token exists, then check for a custom help message
    char *cmd_arg_token = strtok(NULL, " \t\r\n");

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
                "Command: -callback [index] [count] [payload]\n\r"
                "| args:\n\r"
                "| | index: Callback index (0 for timer, 1 for SW1, 2 for SW2).\r\n"
                "| | count: Number of times to execute the payload (-1 for infinite).\r\n"
                "| | payload: Command to execute when the event occurs.\r\n"
                "| Description: Configures a callback to execute a payload when an event occurs.\r\n"
                "| Example usage: \"-callback 1 2 -gpio 3 t\" -> On SW1 press, toggle GPIO 3 twice.\r\n";

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
    else if (strcmp(cmd_arg_token,      "timer") == 0 || strcmp(cmd_arg_token,           "-timer") == 0) {
            helpMessage =
               //================================================================================ <-80 characters
                "Command: -timer [period_us]\n\r"
                "| args:\n\r"
                "| | period_us: The period of Timer0 in microseconds. Minimum value is 100 us\r\n"
                "| Description: Sets the period of Timer0 to the specified period in microseconds.\r\n"
                "| |            If no period is specified, no change is made to Timer0.\r\n"
                "| Example usage: \"-timer 100000\" -> Sets Timer0 period to 100,000 us (100 ms).\r\n";
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
                "| |            repeat it.\r\n"
                "| Example usage: \"-ticker 3 100 100 5 -gpio 2 t\" -> Uses ticker 3, waits 1000\r\n"
                "| |              ms, then toggles GPIO 2 every 1000 ms, repeating 5 times.\r\n";
    }
    else {
        helpMessage =
              //================================================================================ <-80 characters
                "| Supported Command [arg1][arg2]...   |  Command Description\r\n"
                "-------------------------------------------------------------------------------\r\n"
                "| -about                              :  Display program information.\r\n"
                "| -callback  [index][count][payload]  :  Configures a callback to execute a\r\n"
                "|                                        payload when an event occurs.\r\n"
                "| -error                              :  Displays number of times that each\r\n"
                "|                                        error type has triggered.\r\n"
                "| -gpio      [pin][function][val]     :  Performs pin function on selected"
                "|                                        GPIO.\r\n"
                "| -help      [command]                :  Display this help message or a\r\n"
                "|                                        specific message based on command\r\n"
                "|                                     :  to a command. I.E \"-help print\"\r\n"
                "| -memr      [address]                :  Display contents of given memory\r\n"
                "|                                        address.\r\n"
                "| -print     [string]                 :  Display inputted string.\r\n"
                "|                                        I.E \"-print abc\"\r\n"
                "| -ticker    [index][initialDelay]    :  Configures a ticker to execute a payload\r\n"
                "|            [period][count][payload]    after a delay and repeat it.\r\n"
                "| -timer     [period_us]              :  Sets the period of Timer0 in\r\n"
                "|                                        microseconds.\r\n";
    }

    //UART_write_safe(helpMessage, strlen(helpMessage));
    AddOutMessage(helpMessage);
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
    if(memaddr >= 0x100000 && memaddr < 0x20000000)     // Too high for FLASH too low for SRAM
        goto ERROR38;
    if(memaddr >= 0x20040000)  // Too high for SRAM
        goto ERROR38;

    // Header and Surrounding Addresses
    sprintf(msg, "MEMR\r\n");
    //UART_write_safe(msg, strlen(msg));
    AddOutMessage(msg);
    sprintf(msg, "%#010x %#010x %#010x %#010x\r\n", memaddr+0xC, memaddr+0x8, memaddr+0x4, memaddr);
    //UART_write_safe(msg, strlen(msg));
    AddOutMessage(msg);

    // Content of Surrounding Addresses
    val = *(int32_t *) (memaddr + 0xC);
    sprintf(msg, "%#010x ", val);
    //UART_write_safe(msg, strlen(msg));
    AddOutMessage(msg);
    val = *(int32_t *) (memaddr + 0x8);
    sprintf(msg, "%#010x ", val);
    //UART_write_safe(msg, strlen(msg));
    AddOutMessage(msg);
    val = *(int32_t *) (memaddr + 0x4);
    sprintf(msg, "%#010x ", val);
    //UART_write_safe(msg, strlen(msg));
    AddOutMessage(msg);
    val = *(int32_t *) (memaddr + 0x0);
    sprintf(msg, "%#010x\r\n", val);
    //UART_write_safe(msg, strlen(msg));
    AddOutMessage(msg);

    // Display last byte (8 bits) of memorig address
    uint8_t last_byte = *(uint8_t *) memorig;
    sprintf(msg, "%#04x\n\r", last_byte);  // Display the last byte in hexadecimal
    //UART_write_safe(msg, strlen(msg));
    AddOutMessage(msg);
    return;

ERROR38:
    //UART_write_safe_strlen(raiseError(ERR_ADDR_OUT_OF_RANGE));
    AddOutMessage(raiseError(ERR_ADDR_OUT_OF_RANGE));
}

// Prints the message after the first token, which should be "-print"
void CMD_print() {
    //get first content after "> -print"
    char *msg_token = strtok(NULL, "\r\n");

    if(msg_token) {
        //UART_write_safe(msg_token, strlen(msg_token));
        AddOutMessage(msg_token);
    }
}

// Command function to parse and set up a ticker
void CMD_ticker() {
    // Parse index
    char *index_token = strtok(NULL, " \t\r\n");
    int index = -1;
    if (index_token) {
        index = atoi(index_token);
    }
    if (index < 0 || index >= MAX_TICKERS) {
        AddOutMessage(raiseError(ERR_INVALID_TICKER_INDEX));
        return;
    }

    // Parse initial delay
    char *delay_token = strtok(NULL, " \t\r\n");
    uint32_t initialDelay = 0;
    if (delay_token) {
        initialDelay = strtoul(delay_token, NULL, 10);
    } else {
        AddOutMessage(raiseError(ERR_MISSING_DELAY_PARAMETER));
        return;
    }

    // Parse period
    char *period_token = strtok(NULL, " \t\r\n");
    uint32_t period = 0;
    if (period_token) {
        period = strtoul(period_token, NULL, 10);
    } else {
        AddOutMessage(raiseError(ERR_MISSING_PERIOD_PARAMETER));
        return;
    }

    // Parse count
    char *count_token = strtok(NULL, " \t\r\n");
    int32_t count = -1;
    if (count_token) {
        count = atoi(count_token);
    } else {
        AddOutMessage(raiseError(ERR_MISSING_COUNT_PARAMETER));
        return;
    }

    // The rest is the payload
    char *payload_start = strtok(NULL, "\r\n");
    if (!payload_start) {
        AddOutMessage(raiseError(ERR_MISSING_PAYLOAD));
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
    AddOutMessage(msg);
}

void CMD_timer() {

    // Next parameter should be the period in microseconds
    char *val_token = strtok(NULL, " \t\r\n");
    char *val_ptr;
    uint32_t val_us;  // Value of timer period in us

    if(!val_token) {
        //UART_write_safe_strlen("Warning: No period value detected and no change made to Timer0.\r\n");
        AddOutMessage("Warning: No period value detected and no change made to Timer0.\r\n");
        return; // Do nothing if there is no input (Or do we need to throw an error?)
    }
    else
        val_us = strtoul(val_token, &val_ptr, 10);

    // Throw error if value is too low
    if(val_us < MIN_TIMER_PERIOD_US) {
        //UART_write_safe_strlen(raiseError(ERR_INVALID_TIMER_PERIOD));
        AddOutMessage(raiseError(ERR_INVALID_TIMER_PERIOD));
        return; // No change
    }
    //UART_write_safe_strlen("Time to write...\r\n");
    AddOutMessage("Time to write...\r\n");

    Timer_stop(glo.Timer0);
    glo.Timer0Period = val_us;
    Timer_setPeriod(glo.Timer0, Timer_PERIOD_US, val_us);
    int32_t status = Timer_start(glo.Timer0);

    if(status == Timer_STATUS_ERROR) {
        //UART_write_safe_strlen(raiseError(ERR_TIMER_STATUS_ERROR));
        AddOutMessage(raiseError(ERR_TIMER_STATUS_ERROR));
    }
    else {
        char output_msg[BUFFER_SIZE];
        sprintf(output_msg, "Set Timer0 period to %d us\r\n", val_us);
        //UART_write_safe_strlen(output_msg);
        AddOutMessage(output_msg);
    }
}



