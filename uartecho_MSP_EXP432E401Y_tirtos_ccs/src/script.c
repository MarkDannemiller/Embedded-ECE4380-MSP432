#include "script.h"
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
    AddProgramMessage("| Line | Script Contents                             | Memory Range            |\r\n");
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

void execute_script_from_line(int line_number) {
    int i;
    for (i = line_number; i < SCRIPT_LINE_COUNT; i++) {
        if (scriptLines[i][0] == '\0') {
            // Stop execution when an empty line is encountered
            break;
        }
        // Queue the script line as a payload
        AddPayload(scriptLines[i]);
    }
}
