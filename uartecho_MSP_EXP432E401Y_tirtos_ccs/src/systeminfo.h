#include <stdio.h>  // Include this for sprintf
#include "p100.h"

#define VERSION 9
#define SUBVERSION 0
#define TERMINAL_NAME "MARK-DANNEMILLER-MSP432"

#define ASSIGNMENT "P900"

//================================================
// System Messages
//================================================

//                          .------------------------------_-----------------------------
#define ABOUT_HEADER        "\n\r%s Terminal Version v%d.%d\r\n"
#define COMPILE_DATETIME    "Compiled on: %s %s\r\n"


char* formatAboutHeaderMsg() {
    // Static buffer to hold the formatted message
    static char formattedMessage[MAX_MESSAGE_SIZE];

    // Use sprintf to format the string with TERMINAL_NAME, VERSION, and SUBVERSION
    sprintf(formattedMessage, ABOUT_HEADER, TERMINAL_NAME, VERSION, SUBVERSION);
    return formattedMessage;
}

char* formatCompileDateTime() {
    // Static buffer to hold the formatted message
    static char formattedMessage[MAX_MESSAGE_SIZE];
    sprintf(formattedMessage, COMPILE_DATETIME, __DATE__, __TIME__);
    return formattedMessage;
}
