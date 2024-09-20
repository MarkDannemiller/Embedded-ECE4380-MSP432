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

// System headers
#include "p100.h"
#include "systeminfo.h"

#ifdef Globals
extern Globals glo;
#endif


// Initialize Global Struct from header file
void init_globals() {
    glo.cursor_pos = 2;
    glo.msg_size = 2;
    glo.msgBuffer[0] = '>';
    glo.msgBuffer[1] = ' ';
    glo.msgBuffer[2] = '\0';
}


/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    init_globals();
    init_drivers();

    if (glo.uart == NULL) {
        /* UART_open() failed */
        while (1);
    }

    clear_console();

    // Send system info at start of UART
    char* initMsg = formatAboutHeaderMsg();
    char* compileTimeMsg = formatCompileDateTime();

    UART_write_safe(initMsg, strlen(initMsg));
    UART_write_safe(compileTimeMsg, strlen(compileTimeMsg));
    reset_buffer();
    refresh_user_input();

    /* Loop forever handling UART input and responses */
    while (1) {
        handle_UART();
    }
}


void init_drivers() {

    /* Call driver init functions */
    GPIO_init();
    UART_init();

    // TODO write function to return configuration of pin (INPUT/OUTPUT) and another to get state (HIGH/LOW)
    // TODO write command to read/write/toggle pin based on the numbers below:
    // **TODO map GPIO to indexed array (CONFIG_GPIO_LED_0 = 0, GPIO_4 = 4, etc)


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

    // INPUTS
    /* Configure the SWITCH 6 pin */
    //GPIO_setConfig(CONFIG_GPIO_SWITCH_6, GPIOMSP432E4_PJ0 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING); // TODO make sure this is correct input init
    /* Configure the SWITCH 7 pin */
    //GPIO_setConfig(CONFIG_GPIO_SWITCH_7, GPIOMSP432E4_PJ1 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING); // TODO make sure this is correct input init


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

    // TODO init timer0
}



//================================================
// UART Comms
//================================================

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
    if (UART_write(glo.uart, message, size) == UART_STATUS_ERROR) {
        UART_write_safe_strlen(raiseError(ERR_MSG_WRITE_FAILED));
        //UART_write(glo.uart, "Error: Failed to write message.\r\n", 31);
        return false;
    }

    return true;  // Return true if all checks pass and write is successful
}

// Writes without needing the size.  Uses strlen() to get size
bool UART_write_safe_strlen(const char *message) {
    int size = strlen(message);
    return UART_write_safe(message, size);
}


void refresh_user_input() {
    UART_write_safe(CLEAR_LINE_RESET, sizeof(CLEAR_LINE_RESET) - 1);
    UART_write_safe(glo.msgBuffer, glo.msg_size+1);
}

void handle_UART() {
    char key_in;

    // Catch cursor out of bounds
    if(glo.cursor_pos < 2) {
        glo.cursor_pos = 2;
    }

    UART_read(glo.uart, &key_in, 1);
    glo.msgBuffer[glo.cursor_pos] = key_in;


    if (key_in == '\r' || key_in == '\n') {  // Enter key

        //TODO see if this is best for handling no command
        if(glo.cursor_pos > 2) {
            // Process the completed input
            process_input();
        }
        reset_buffer();
    } else if (key_in == '\b' || key_in == 0x7F) {  // Backspace key
        if (glo.cursor_pos > 2) {
            // Remove the last character
            glo.msgBuffer[--glo.cursor_pos] = '\0';
        }
        //TODO investigate why this code prevents backspace into "> "
        if(glo.cursor_pos == 2) {
            glo.cursor_pos = 2;
            glo.msg_size = 2;
            glo.msgBuffer[0] = '>';
            glo.msgBuffer[1] = ' ';
            glo.msgBuffer[2] = '\0';
        }
    } else if (key_in == '\033') {  // Escape character for arrow keys
        // Implement handling of arrow keys if needed

    // Normal Operation
    } else {
        // Add the character to the buffer if it's not full
        if (glo.cursor_pos < BUFFER_SIZE - 1) {
            glo.msgBuffer[glo.cursor_pos++] = key_in;
            glo.msgBuffer[glo.cursor_pos] = '\0';
        }
        else {
            UART_write_safe("\n\r", 2);  // Move to a new line to display output
            UART_write_safe_strlen(raiseError(ERR_BUFFER_OF));
            //UART_write_safe("Error: Buffer Overflow.\r\n", 25);  // Move to a new line to display output
            reset_buffer();
        }
    }

    glo.msg_size = glo.cursor_pos;//= glo.cursor_pos > glo.msg_size ? glo.cursor_pos : glo.msg_size;
    refresh_user_input();

    // Increment cursor only if in bounds of buffer
}

void reset_buffer() {
    glo.cursor_pos = 2;
    glo.msg_size = 2;
    glo.msgBuffer[0] = '>';
    glo.msgBuffer[1] = ' ';
    glo.msgBuffer[2] = '\0';
    int i;
    //TODO dont do this -> clear buffer in better way?
    for(i=3; i<BUFFER_SIZE; i++){
        glo.msgBuffer[i] = 0;
    }

    UART_write(glo.uart, NEW_LINE_RETURN, sizeof(NEW_LINE_RETURN) - 1);
}

// Handle processing commands and call associated functions
void process_input() {

    UART_write_safe("\r\n", 2);  // Move to a new line to display output

    // Copy the original message buffer to preserve its contents
    char msgBufferCopy[BUFFER_SIZE];
    strncpy(msgBufferCopy, glo.msgBuffer, BUFFER_SIZE);

    // Start tokenizing the input message buffer copy
    char *saveptr;
    char *token = strtok(msgBufferCopy, " \t\r\n");
    char *p;  // Declare p at the beginning of the block

    bool commandRecognized = false;  // To track if any valid command is recognized

    token = strtok(NULL, " \t\r\n");

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
        //print_memr(saveptr);
        CMD_print();
        commandRecognized = true;
    }
    //=========================================================================

    if (!commandRecognized) {  // Only send error if no valid command has been found yet
        UART_write_safe_strlen(raiseError(ERR_UNKNOWN_COMMAND));
        //UART_write_safe("Error: Unknown command. Try -help for list of commands.\n\r", strlen("Error: Unknown command. Try -help for list of commands.\r\n"));
    }
}


void clear_console() {
    UART_write_safe(CLEAR_CONSOLE, strlen(CLEAR_CONSOLE));
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

    UART_write_safe(aboutMessage, strlen(aboutMessage));
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
    else if (strcmp(cmd_arg_token,      "print") == 0  || strcmp(cmd_arg_token,           "-print") == 0) {
        helpMessage =
               //================================================================================ <-80 characters
                "Command: -print [message]\n\r"
                "| args:\n\r"
                "| | messsage: The contents to print. Displays the entire line after \"-print \".\r\n"
                "| Description: Displays the message inputted after command statement.\r\n"
                "| Example usage: \"-print abc\" -> Displays the contents of address 0x1000.\r\n";
    }
    else {
        helpMessage =
              //================================================================================ <-80 characters
                "Supported commands      |  Command Description        \r\n"
                "-------------------------------------------------------\r\n"
                "| -about                :  Display program information.\r\n"
                "| -help  [arg command]  :  Display this help message or specific message to a \r\n"
                "|                       :  command. I.E \"-help print\"\r\n"
                "| -print [arg string]   :  Display inputted string.  I.E \"-print abc\"\r\n"
                "| -memr  [arg address]  :  Display contents of given memory address.\r\n";
    }


    UART_write_safe(helpMessage, strlen(helpMessage));
}

// Prints the message after the first token, which should be "-print"
void CMD_print() {
    //get first content after "> -print"
    char *msg_token = strtok(NULL, "\r\n");

    if(msg_token) {
        UART_write_safe(msg_token, strlen(msg_token));
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
    if(memaddr >= 0x100000 && memaddr < 0x20000000)     // Too high for FLASH too low for SRAM
        goto ERROR38;
    if(memaddr >= 0x20040000)  // Too high for SRAM
        goto ERROR38;

    // Header and Surrounding Addresses
    sprintf(msg, "MEMR\r\n");
    UART_write_safe(msg, strlen(msg));
    sprintf(msg, "%#010x %#010x %#010x %#010x\r\n", memaddr+0xC, memaddr+0x8, memaddr+0x4, memaddr);
    UART_write_safe(msg, strlen(msg));

    // Content of Surrounding Addresses
    val = *(int32_t *) (memaddr + 0xC);
    sprintf(msg, "%#010x ", val);
    UART_write_safe(msg, strlen(msg));
    val = *(int32_t *) (memaddr + 0x8);
    sprintf(msg, "%#010x ", val);
    UART_write_safe(msg, strlen(msg));
    val = *(int32_t *) (memaddr + 0x4);
    sprintf(msg, "%#010x ", val);
    UART_write_safe(msg, strlen(msg));
    val = *(int32_t *) (memaddr + 0x0);
    sprintf(msg, "%#010x\r\n", val);
    UART_write_safe(msg, strlen(msg));

    // Display last byte (8 bits) of memorig address
    uint8_t last_byte = *(uint8_t *) memorig;
    sprintf(msg, "%#04x\n\r", last_byte);  // Display the last byte in hexadecimal
    UART_write_safe(msg, strlen(msg));
    return;

ERROR38:
    UART_write_safe_strlen(raiseError(ERR_ADDR_OUT_OF_RANGE));
    //UART_write_safe("Error: Address out of range.", 28);
}

void CMD_error() {
    char str[BUFFER_SIZE];
    int i;

    UART_write_safe_strlen("Error Count:\r\n");
    for(i = 0; i < ERROR_COUNT; i++) {
        sprintf(str, "|  %s: %d\r\n", getErrorName(i), errorCounters[i]);
        UART_write_safe_strlen(str);
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
        UART_write_safe_strlen(raiseError(ERR_GPIO_OUT_OF_RANGE));
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

    UART_write_safe_strlen(output_msg);
}

