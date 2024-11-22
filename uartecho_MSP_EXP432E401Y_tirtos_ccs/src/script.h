#define SCRIPT_LINE_COUNT 64
#define SCRIPT_LINE_SIZE 256

char scriptLines[SCRIPT_LINE_COUNT][SCRIPT_LINE_SIZE];


void init_script_lines();
void print_all_script_lines();
void print_script_line(int line_number);
void execute_script_from_line(int line_number);