/*
 *  ======== script.c ========
 */
#include "script.h"
#include "register.h"
#include "p100.h"

void init_script_lines() {
    int i;
    for (i = 0; i < SCRIPT_LINE_COUNT; i++) {
        scriptLines[i][0] = '\0';  // Set first character to null terminator
    }
}

void print_all_script_lines() {
    AddProgramMessage("================================= Script Lines =================================\r\n");
    AddProgramMessage("| Line | Script Contents                             | Memory Range            |\r\n");
    AddProgramMessage("|------|---------------------------------------------|-------------------------|\r\n");
    int i;
    for (i = 0; i < SCRIPT_LINE_COUNT; i++) {
        char msg[500];
        //sprintf(msg, "| %-11d | %-39s |\r\n", i, scriptLines[i]);
        sprintf(msg, "| %-4d | %-43s | 0x%08X - 0x%08X |\r\n", i, scriptLines[i], (unsigned int)(uintptr_t)&scriptLines[i][0], (unsigned int)(uintptr_t)&scriptLines[i][SCRIPT_LINE_SIZE-1]);
        AddProgramMessage(msg);
    }
    AddProgramMessage("================================================================================\r\n");
}

void print_script_line(int line_number) {
    AddProgramMessage("================================= Script Line ==================================\r\n");
    AddProgramMessage("| Line | Script Contents                             | Memory Range            \r\n");
    AddProgramMessage("|------|---------------------------------------------|-------------------------|\r\n");
    char msg[500];
    if (scriptLines[line_number][0] != '\0') {
        sprintf(msg, "| %-4d | %-43s | 0x%08X - 0x%08X |\r\n", line_number, scriptLines[line_number], (unsigned int)(uintptr_t)&scriptLines[line_number][0], (unsigned int)(uintptr_t)&scriptLines[line_number][SCRIPT_LINE_SIZE-1]);
    } else {
        sprintf(msg, "| %-4d | %-43s | 0x%08X - 0x%08X |\r\n", line_number, "Empty", (unsigned int)(uintptr_t)&scriptLines[line_number][0], (unsigned int)(uintptr_t)&scriptLines[line_number][SCRIPT_LINE_SIZE-1]);
    }
    AddProgramMessage(msg);
    AddProgramMessage("================================================================================\r\n");
}

// Execute the script starting from the specified line number
void execute_script_from_line(int line_number) {
    // Check for valid script line
    if (line_number < 0 || line_number >= SCRIPT_LINE_COUNT || !scriptLines[line_number]) {
        return; // No script to execute
    }

    if(scriptLines[line_number][0] == '\0') {
        return; // Empty script line
    }

    // Set scriptPointer to the starting line
    glo.scriptPointer = line_number;

    // Allow the first line to execute.  Otherwise, the task will wait for the semaphore from regular payloads
    Semaphore_post(glo.bios.PayloadSem);

    // // Add the first line to the PayloadQueue
    // AddPayload(scriptLines[line_number]);
}

//=============================================================================
// Conditionals
//=============================================================================

// Get the value of an operand
int getOperandValue(const char *operand) {
    // Check if it's an immediate value
    if (operand[0] == '#') {
        // Immediate value
        char *endptr;
        int value = strtol(operand + 1, &endptr, 0);
        if (*endptr != '\0') {
            AddProgramMessage("Error: Invalid immediate value.\r\n");  // TODO: Add to errors
            return INT64_MIN;
        }
        return value;
    } else {
        // Should be a register
        int regIndex = parse_register(operand);
        if (regIndex < 0 || regIndex >= NUM_REGISTERS) {
            AddProgramMessage("Error: Invalid register.\r\n");  // TODO: Add to errors
            return INT64_MIN;
        }
        return registers[regIndex];
    }
}

// Execute the destination of a conditional
void execute_destination(const char *dest) {
    // Trim leading and trailing whitespaces
    char destCopy[BUFFER_SIZE];
    strncpy(destCopy, dest, BUFFER_SIZE);
    trim(destCopy);

    // Check if destination is empty
    if (strlen(destCopy) == 0) {
        return; // Do nothing
    }

    // Since the destination can be any payload, we can simply add it to the payload queue
    execute_payload(destCopy);
}

// Trim leading and trailing whitespaces
void trim(char *str) {
    // Trim leading spaces
    while(isspace((unsigned char)*str)) str++;

    // Trim trailing spaces
    char *end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = '\0';
}
