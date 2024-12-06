#ifndef P100_H  // Include guard to prevent multiple inclusions
#define P100_H

// System Header files
#include <xdc/std.h> /* initializes XDCtools */
#include <stdbool.h>  // Include this header to use bool, true, and false

// Driver Header files
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/Timer.h>
#include "ti_drivers_config.h"

// BIOS Header files
#include <ti/sysbios/BIOS.h> /* initializes SYS/BIOS */
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/gates/GateSwi.h>

// User defined headers
#include "audio.h"

// NETUDP
#define NetQueueLen 32
#define NetQueueSize 320
#define REG_DIAL1 0
#define REG_DIAL2 1
#define DEFAULTPORT 1000

#define BUFFER_SIZE 80          // Maximum input buffer by user (check whether to increase?)
#define MAX_LINE_LENGTH 80      // Maximum line length before wrapping
#define MAX_MESSAGE_SIZE 2000   // Maximum output message size
#define MAX_QUEUE_SIZE 500      // Maximum number of messages in queue

#define CLEAR_LINE_RESET "\033[2K\033[1G\r"
#define NEW_LINE_RETURN "\r\n"
#define CLEAR_CONSOLE "\033[2J\033[H"

#define MIN_TIMER_PERIOD_US 100

// Extern declarations for BIOS objects created in .cfg
extern Task_Handle UARTReader0;
extern Task_Handle UARTWriter;
extern Task_Handle UARTReader1;
extern Task_Handle PayloadExecutor;
extern Task_Handle TickerProcessor;

extern Semaphore_Handle UARTWriteSem;
extern Semaphore_Handle PayloadSem;
extern Semaphore_Handle TickerSem;
extern Semaphore_Handle ADCSemaphore;
extern Semaphore_Handle NetSemaphore;

extern Queue_Handle PayloadQueue;
extern Queue_Handle OutMsgQueue;

extern Swi_Handle Timer0_swi;
extern Swi_Handle SW1_swi;
extern Swi_Handle SW2_swi;

extern GateSwi_Handle gateSwi0;  // Timer and audio gateSwi
extern GateSwi_Handle gateSwi1;  // Payload gateSwi
extern GateSwi_Handle gateSwi2;  // Callback gateSwi
extern GateSwi_Handle gateSwi3;  // OutMessage gateSwi
extern GateSwi_Handle gateSwi4;  // NetworkGate gateSwi


typedef struct PayloadMessage {
    Queue_Elem elem;
    char *data;
    bool isProgramOutput;
} PayloadMessage, *PMsg;

typedef struct NetOutQ {
    int32_t payloadWriting, payloadReading;
    char    payloads[NetQueueLen][NetQueueSize];
    int32_t binaryCount[320];
} NetOutQ;

typedef struct Discoveries{
    uint32_t IP_address;
} Discoveries;


// List of BIOS objects for RTOS
typedef struct {

    Task_Handle UARTWriter;                 // Created statically in release.cfg
    Task_Handle UARTReader0;                 // Created statically in release.cfg

    Task_Handle UARTReader1;                 // Created statically in release.cfg

    Task_Handle PayloadExecutor;            // Created statically in release.cfg
    Task_Handle TickerProcessor;

    Queue_Handle PayloadQueue;
    Semaphore_Handle PayloadSem;
    Semaphore_Handle NetSemaphore;

    Queue_Handle OutMsgQueue;
    Semaphore_Handle UARTWriteSem;           // Created statically in release.cfg

    Semaphore_Handle TickerSem;
    Semaphore_Handle ADCSemaphore;

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

    // UART Console
    char inputBuffer_uart0[BUFFER_SIZE];
    int cursor_pos;                 // Cursor position of cursor in the input buffer
    int progOutputCol;              // Program output column position
    int progOutputLines;            // Number of lines program output occupies
    UART_Handle uart0;
    UART_Params uartParams0;

    // TODO look at switching to callbacks for uart0 and uart1
    // UART1 for outputting payloads to another device
    char inputBuffer_uart1[BUFFER_SIZE];
    UART_Handle uart1;
    UART_Params uartParams1;
    int cursor_pos_uart1;

    Timer_Handle Timer0;
    Timer_Params timer0_params;
    int32_t Timer0Period;

    AudioController audioController;

    NetOutQ  NetOutQ;
    Discoveries Discoveries[32];
    uint32_t Multicast;

    BiosList bios;
    
    int scriptPointer;

    bool emergencyStopActive;
    Semaphore_Handle emergencyStopSem;

    uint32_t integrityTail;
} glo;


void init_globals();
void init_drivers();


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

    ERR_INVALID_SCRIPT_LINE,
    ERR_MISSING_SCRIPT_COMMAND,
    ERR_UNKNOWN_SCRIPT_OP,

    ERR_INVALID_IF_SYNTAX,
    ERR_MISSING_DESTINATION,
    ERR_INVALID_CONDITION,
    ERR_UART1_WRITE_FAILED,

    ERROR_COUNT // Keeps track of the number of error types
} Errors;


extern const char* errorMessages[];
extern const char* errorNames[];
extern int errorCounters[ERROR_COUNT];


void emergency_stop();

//================================================
// Utility
//================================================

char *memory_strdup(const char *src);                   // Duplicate a string in memory
int isPrintable(char ch);                               // ASCII printable characters
bool isNumeric(const char *str);                        // Check if a string is numeric.
double parseDouble(const char *str, bool *success);     // Parse a double from a string
char *strtok_r(char *str, const char *delim, char **saveptr);  // Custom reentrant strtok implementation
char* NextSubString(char *msg, bool Print);
bool MatchSubString(const char *needle, const char *haystack);


char *UDPParse(char *buff, struct sockaddr_in *clientAddr, bool todash);

//================================================
// Drivers
//================================================

typedef enum { LOW, HIGH } pinState; // Simple language for HIGH and LOW pin states


extern const int pin_count;
extern const int pinMap[];

void togglePin(int pin_num);
void digitalWrite(int pin_num, pinState state);
pinState digitalRead(int pin_num);
const char *getErrorName(Errors error);


//================================================
// UART0 Comms
//================================================

// UART Console Functions (Called by UARTReader0)
void refresh_user_input();
void handle_UART0();
void reset_buffer();
void clear_console();

void refreshUserInputLine();
void moveCursorToColumn(int col);
void moveCursorUp(int lines);
void moveCursorDown(int lines);


// Outward Messages (Called by UARTWriter)
void AddProgramMessage(char *data);
bool UART_write_safe(const char *message, int size);
bool UART_write_safe_strlen(const char *message);

//================================================
// UART1 Comms
//================================================

// UART1 Comms (Called by UARTReader1)
void handle_UART1();
void reset_buffer_uart1();


//================================================
// Payload Command Handling
//================================================

// Payload Handling (Called by PayloadExecutor)
void AddPayload(char *payload);  // Should this use gates to block swi? Nuter does with his AddPayload() function
void execute_payload(char *msg);

void CMD_about(char **saveptr);       // Print about / system info
void CMD_audio(char **saveptr);       // Generate audio sample and send to DAC over SPI
void CMD_callback(char **saveptr);    // Configure a callback for timer or GPIO events
void CMD_error(char **saveptr);       // Display count of each error type
void CMD_gpio(char **saveptr);        // Read/Write/Toggle inputed GPIO pin
void CMD_help(char **saveptr);        // Print help info about all/specific command(s)
void CMD_if(char **saveptr);          // Conditional execution of payload
void CMD_memr(char **saveptr);        // Display contents of memory address
void CMD_print(char **saveptr);       // Print inputed string
void CMD_reg(char **saveptr);         // Perform register operations
void CMD_rem(char **saveptr);         // Add comments or remarks in scripts
void CMD_script(char **saveptr);      // Handle operations related to loading and executing scripts
void CMD_sine(char **saveptr);        // Generate a sine wave sample or set the frequency for continuous generation with sample rate based on timer0 period
void CMD_timer(char **saveptr);       // Sets the periodic timer0 period
void CMD_ticker(char **saveptr);      // Configures ticker and payload
void CMD_uart(char **saveptr);        // Send payload to UART1
void ParseVoice(char *ch);       // Copies the given 128 samples into the correct TX buffers and applies correction logic.
void CMD_sus(char **saveptr);         // The imposter is sus

// NETUDP
void ParseNetUDP(char *ch, int32_t binaryCount);

#endif  // End of include guard
