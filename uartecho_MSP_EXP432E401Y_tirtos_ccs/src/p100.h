#ifndef P100_H  // Include guard to prevent multiple inclusions
#define P100_H

// Driver Header files
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/Timer.h>

#include <xdc/std.h> /* initializes XDCtools */
#include <ti/sysbios/BIOS.h> /* initializes SYS/BIOS */
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Swi.h>

#include "ti_drivers_config.h"


#include <stdbool.h>  // Include this header to use bool, true, and false

#define BUFFER_SIZE 80          // Maximum input buffer by user
#define MAX_MESSAGE_SIZE 2000   // Maximum output message size
#define MAX_QUEUE_SIZE 500      // Maximum number of messages in queue

#define CLEAR_LINE_RESET "\033[2K\033[1G\r"
#define NEW_LINE_RETURN "\r\n"
#define CLEAR_CONSOLE "\033[2J\033[H"

#define MIN_TIMER_PERIOD_US 100

// Extern declarations for BIOS objects created in .cfg
extern Task_Handle UARTReader;
extern Task_Handle UARTWriter;
extern Task_Handle PayloadExecutor;
extern Task_Handle TickerProcessor;

extern Semaphore_Handle UARTWriteSem;
extern Semaphore_Handle PayloadSem;
extern Semaphore_Handle TickerSem;

extern Queue_Handle PayloadQueue;
extern Queue_Handle OutMsgQueue;

extern Swi_Handle Timer0_swi;
extern Swi_Handle SW1_swi;
extern Swi_Handle SW2_swi;


typedef struct PayloadMessage {
    Queue_Elem elem;
    char *data;
} PayloadMessage, *PMsg;


// List of BIOS objects for RTOS
typedef struct {

    Task_Handle UARTWriter;                 // Created statically in release.cfg
    Task_Handle UARTReader;                 // Created statically in release.cfg
    Task_Handle PayloadExecutor;            // Created statically in release.cfg
    Task_Handle TickerProcessor;

    Queue_Handle PayloadQueue;
    Semaphore_Handle PayloadSem;

    Queue_Handle OutMsgQueue;
    Semaphore_Handle UARTWriteSem;           // Created statically in release.cfg

    Semaphore_Handle TickerSem;

    Swi_Handle Timer0_swi;
    Swi_Handle SW1_swi;
    Swi_Handle SW2_swi;
} BiosList;

//================================================
// Global variables
//================================================
struct Globals {
    uint32_t integrityHead;
    uint32_t byteSize;  // Size of Globals
    char msgBuffer[BUFFER_SIZE];
    int cursor_pos;
    int msg_size;
    UART_Handle uart;
    UART_Params uartParams;

    Timer_Handle Timer0;
    Timer_Params timer0_params;
    int32_t Timer0Period;

    BiosList bios;
    uint32_t integrityTail;
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
    ERR_INVALID_TIMER_PERIOD,
    ERR_TIMER_STATUS_ERROR,
    ERR_UART_COLLISION,
    ERR_INVALID_CALLBACK_INDEX,
    ERR_MISSING_COUNT_PARAMETER,
    ERR_MISSING_PAYLOAD,
    ERR_INVALID_TICKER_INDEX,
    ERR_MISSING_DELAY_PARAMETER,
    ERR_MISSING_PERIOD_PARAMETER,
    ERR_PAYLOAD_QUEUE_OF,
    ERR_OUTMSG_QUEUE_OF,
    ERROR_COUNT // Keeps track of the number of error types
} Errors;


extern const char* errorMessages[];
extern const char* errorNames[];
extern int errorCounters[ERROR_COUNT];


//================================================
// Drivers
//================================================

typedef enum { LOW, HIGH } pinState; // Simple language for HIGH and LOW pin states


extern const int pin_count;
extern const int pinMap[];

void togglePin(int pin_num);
void digitalWrite(int pin_num, pinState state);
pinState digitalRead(int pin_num);
const char* getErrorName(Errors error);



//================================================
// UART Comms
//================================================

// UART Console Functions (Called by UARTReader)
void refresh_user_input();
void handle_UART();
void reset_buffer();
void clear_console();


// Outward Messages (Called by UARTWriter)
void AddOutMessage(char *data);
bool UART_write_safe(const char *message, int size);
bool UART_write_safe_strlen(const char *message);


//================================================
// Payload Command Handling
//================================================

// Payload Handling (Called by PayloadExecutor)
void AddPayload(char *payload);  // Should this use gates to block swi? Nuter does with his AddPayload() function
void execute_payload(char *msg);

void CMD_about();       // Print about / system info
void CMD_callback();    // Configure a callback for timer or GPIO events
void CMD_error();       // Display count of each error type
void CMD_gpio();        // Read/Write/Toggle inputed GPIO pin
void CMD_help();        // Print help info about all/specific command(s)
void CMD_memr();        // Display contents of memory address
void CMD_print();       // Print inputed string
void CMD_timer();       // Sets the periodic timer0 period
void CMD_ticker();      // Configures ticker and payload

// TODO these prints and tie into commands when no args are given
void print_timer_info();
void print_ticker_info();
void print_callback_info();


#endif  // End of include guard
