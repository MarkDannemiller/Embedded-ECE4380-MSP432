#ifndef P100_H  // Include guard to prevent multiple inclusions
#define P100_H

// Driver Header files
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>

#include <stdbool.h>  // Include this header to use bool, true, and false

#define BUFFER_SIZE 80          // Maximum input buffer by user
#define MAX_MESSAGE_SIZE 1000   // Maximum output message size

#define CLEAR_LINE_RESET "\033[2K\033[1G\r"
#define NEW_LINE_RETURN "\r\n"
#define CLEAR_CONSOLE "\033[2J\033[H"

//================================================
// TODO List
//================================================
//
// Make function to lowercase incoming buffer
// Rewrite print functions to take in a copy of the string, not the string itself (or at least make sure that the string is not destroyed).  We want these functions to run multiple times
// Save commands and get history using arrow keys
//
//================================================


//================================================
// Global variables
//================================================
struct Globals {
    char msgBuffer[BUFFER_SIZE];
    int cursor_pos;
    int msg_size;
    UART_Handle uart;
    UART_Params uartParams;
} glo;


void init_globals();


//================================================
// Error Handling
//================================================


// Define the Errors enum
typedef enum {
    ERR_UNKNOWN_COMMAND,
    ERR_BUFFER_OF,
    ERR_INVALID_MSG_SIZE,
    ERR_NULL_MSG_PTR,
    ERR_EMPTY_MSG,
    ERR_MSG_WRITE_FAILED,
    ERR_ADDR_OUT_OF_RANGE,
    ERR_GPIO_OUT_OF_RANGE,
    ERROR_COUNT // Keeps track of the number of error types
} Errors;

// Define the corresponding error messages
const char* errorMessages[] = {
    "Error: Unknown command. Try -help for list of commands.\r\n",
    "Error: Buffer Overflow\r\n",
    "Error: Invalid message size\r\n",
    "Error: Null message pointer\r\n",
    "Error: Empty message\r\n",
    "Error: Failed to write message\r\n",
    "Error: Address out of range\r\n",
    "Error: GPIO out of range\r\n"
};

const char* errorNames[] = {
    "ERR_UNKNOWN_COMMAND",
    "ERR_BUFFER_OF",
    "ERR_INVALID_MSG_SIZE",
    "ERR_NULL_MSG_PTR",
    "ERR_EMPTY_MSG",
    "ERR_MSG_WRITE_FAILED",
    "ERR_ADDR_OUT_OF_RANGE",
    "ERR_GPIO_OUT_OF_RANGE"
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

void init_drivers();


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

typedef enum { LOW, HIGH } pinState; // Simple language for HIGH and LOW pin states

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


//================================================
// UART Comms
//================================================

bool UART_write_safe(const char *message, int size);
bool UART_write_safe_strlen(const char *message);
void refresh_user_input();
void handle_UART();
void reset_buffer();
void process_input();
void clear_console();

//================================================
// Commands
//================================================

void CMD_about();   // Print about / system info
void CMD_help();    // Print help info about all/specific command(s)
void CMD_print();   // Print inputed string
void CMD_memr();    // Display contents of memory address
void CMD_error();   // Display count of each error type
void CMD_gpio();    // Read/Write/Toggle inputted GPIO pin

#endif  // End of include guard
