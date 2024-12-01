/**
 * @file register.c
 * @brief This file contains the implementation of register operations and parsing functions for use by CMD_reg() in p100.c
 * 
 * The functions provided in this file allow for the initialization and manipulation of registers, 
 * as well as parsing of tokens representing registers, immediate values, and memory addresses. 
 * It includes operations such as move, exchange, increment, decrement, bitwise operations, arithmetic operations, 
 * and finding maximum and minimum values.
 * 
 * The file also includes utility functions to validate memory addresses and parse operands for arithmetic operations.
 * 
 * @note Ensure that the memory addresses used are within the valid range defined for the specific MCU.
 */

#include "register.h"
#include <stdbool.h>
#include "p100.h"

int32_t registers[NUM_REGISTERS] = {0};

/// @brief Initialize registers to 0
void init_registers() {
    int i;
    for (i = 0; i < NUM_REGISTERS; i++) {
        registers[i] = 0;
    }
}

/// @brief Print the values of all registers
void print_all_registers() {
    char msg[BUFFER_SIZE];
    char line[MAX_LINE_LENGTH];

    AddProgramMessage("================================== Registers ===================================\r\n");
    AddProgramMessage("Register | Value       | Address\r\n");
    AddProgramMessage("---------|-------------|----------------\r\n");
    int i;
    for (i = 0; i < NUM_REGISTERS; i++) {
        sprintf(line, "R%-8d| %-11d | 0x%08X\r\n", i, registers[i], (unsigned int)&registers[i]);
        AddProgramMessage(line);
    }
}

/// @brief Returns the register number for a given token
/// @param token Character array containing the token to parse
/// @return Register number if valid, -1 otherwise
int parse_register(char *token) {
    if (!token) return -1;
    // Check for register with 'r' prefix
    if (token[0] == 'r' || token[0] == 'R') {
        int reg_num = atoi(&token[1]);
        if (reg_num >= 0 && reg_num < NUM_REGISTERS) {
            return reg_num;
        }
    // Check for register without 'r' prefix
    } else if (token[0] != '#' && token[0] != '@' && token[0] != '$' && token[0] != 'h') {
        int reg_num = atoi(token);
        if (reg_num >= 0 && reg_num < NUM_REGISTERS) {
            return reg_num;
        }
    }
    return -1; // Invalid register
}

/// @brief Parses immediate values (eg. #10, #0xFF)
/// @param token Character array containing the token to parse
/// @param value Value to store the parsed immediate value
/// @return Whether the token was successfully parsed
bool parse_immediate(char *token, int32_t *value) {
    if (!token || token[0] != '#') return false;
    char *endptr;
    if (token[1] == 'x' || token[1] == 'X'                          // Hexadecimal with 'x' prefix
     || (token[1] == '0' && (token[2] == 'x' || token[2] == 'X'))   // Hexadecimal with '0x' prefix
     || token[1] == 'h' || token[1] == 'H'                          // Hexadecimal with 'h' prefix
     || token[1] == '$') {                                          // Hexadecimal with '$' prefix
        // Hexadecimal immediate (e.g., #xFF, #0xFF, #$FF, #hFF)
        *value = strtol(&token[2], &endptr, 16);
    } else {
        // Decimal immediate (e.g., #10)
        *value = strtol(&token[1], &endptr, 10);
    }
    return (*endptr == '\0');
}

/// @brief Parses memory addresses (eg. @0x1000)
/// @param token Character array containing the token to parse. Either decimal or hexadecimal (with 'x' prefix)
/// @param address The memory address to store the parsed value
/// @return Whether the token was successfully parsed
bool parse_memory_address(char *token, uint32_t *address) {
    if (!token || token[0] != '@') return false;

    int reg_num;
    int32_t immediate_value;
    char *endptr;

    // Check if the token is a register
    reg_num = parse_register(&token[1]);
    if (reg_num != -1) {
        *address = (uint32_t)&registers[reg_num];
        return true;
    }

    // Check if the token is an immediate value
    if (parse_immediate(&token[1], &immediate_value)) {
        *address = (uint32_t)immediate_value;
        return true;
    }

    // Check if the token is a hexadecimal address
    if (token[1] == 'x' || token[1] == 'X') {
        *address = strtoul(&token[2], &endptr, 16);
        return (*endptr == '\0');
    }
    
    return (*endptr == '\0');
}


bool is_valid_memory_address(uint32_t address) {
    // Replace these with your MCU's valid memory ranges
    const uint32_t SRAM_START = 0x20000000;
    const uint32_t SRAM_END = 0x2003FFFF;
    const uint32_t FLASH_START = 0x00000000;
    //const uint32_t FLASH_END = 0x0003FFFF;
    const uint32_t FLASH_END = 0x100000;

    if ((address >= SRAM_START && address <= SRAM_END) ||
        (address >= FLASH_START && address <= FLASH_END)) {
        return true;
    }
    return false;
}



//==============================================================================
// Register Operations
//==============================================================================

/// @brief Move a value to a register, whether it be a register, immediate value, or memory address
/// @param dest_token Character array containing the destination token
/// @param src_token Character array containing the source token
void reg_mov(char *dest_token, char *src_token) {
    int dest_reg = parse_register(dest_token);
    int32_t src_value;

    // Parse operands
    if (!parse_operands(dest_token, src_token, &dest_reg, &src_value)) {
        return;
    }

    registers[dest_reg] = src_value;

    char msg[BUFFER_SIZE];
    sprintf(msg, "R%d = %d\r\n", dest_reg, src_value);
    AddProgramMessage(msg);
}

/// @brief Exchange the values of two registers
/// @param reg1_token Character array containing the first register token
/// @param reg2_token Character array containing the second register token
void reg_xchg(char *reg1_token, char *reg2_token) {
    int reg1 = parse_register(reg1_token);
    int reg2 = parse_register(reg2_token);

    if (reg1 == -1 || reg2 == -1) {
        AddProgramMessage("Error: Invalid register.\r\n");  //  TODO: Add to errors
        return;
    }

    int32_t temp = registers[reg1];
    registers[reg1] = registers[reg2];
    registers[reg2] = temp;

    char msg[BUFFER_SIZE];
    sprintf(msg, "R%d = %d, R%d = %d\r\n", reg1, registers[reg1], reg2, registers[reg2]);
    AddProgramMessage(msg);
}

/// @brief Increment the value in a register
/// @param reg_token Character array containing the register token
void reg_inc(char *reg_token) {
    int reg = parse_register(reg_token);

    if (reg == -1) {
        AddProgramMessage("Error: Invalid register.\r\n");  // TODO: Add to errors
        return;
    }

    registers[reg] += 1;

    char msg[BUFFER_SIZE];
    sprintf(msg, "R%d = %d\r\n", reg, registers[reg]);
    AddProgramMessage(msg);
}

/// @brief Decrement the value in a register
/// @param reg_token Character array containing the register token
void reg_dec(char *reg_token) {
    int reg = parse_register(reg_token);

    if (reg == -1) {
        AddProgramMessage("Error: Invalid register.\r\n");  // TODO: Add to errors
        return;
    }

    registers[reg] -= 1;

    char msg[BUFFER_SIZE];
    sprintf(msg, "R%d = %d\r\n", reg, registers[reg]);
    AddProgramMessage(msg);
}


//==============================================================================
// Logical operations
//==============================================================================

/// @brief Perform bitwise NOT operation on a register
/// @param reg_token Character array containing the register token
void reg_not(char *reg_token) {
    int reg = parse_register(reg_token);

    if (reg == -1) {
        AddProgramMessage("Error: Invalid register.\r\n");  // TODO: Add to errors
        return;
    }

    registers[reg] = ~registers[reg];

    char msg[BUFFER_SIZE];
    sprintf(msg, "R%d = %d\r\n", reg, registers[reg]);
    AddProgramMessage(msg);
}

/// @brief Perform bitwise AND operation on two values and store the result in the destination register
/// @param dest_token Character array containing the destination token
/// @param src_token Character array containing the source token
void reg_and(char *dest_token, char *src_token) {
    int dest_reg;
    int32_t src_value;

    // Parse operands
    if (!parse_operands(dest_token, src_token, &dest_reg, &src_value)) {
        return;
    }

    // Perform bitwise AND operation
    registers[dest_reg] &= src_value;

    // Output the result
    char msg[BUFFER_SIZE];
    sprintf(msg, "R%d = %d\r\n", dest_reg, registers[dest_reg]);
    AddProgramMessage(msg);
}

/// @brief Perform bitwise OR operation on two values and store the result in the destination register
/// @param dest_token Character array containing the destination token
/// @param src_token Character array containing the source token
void reg_ior(char *dest_token, char *src_token) {
    int dest_reg;
    int32_t src_value;

    // Parse operands
    if (!parse_operands(dest_token, src_token, &dest_reg, &src_value)) {
        return;
    }

    // Perform bitwise OR operation
    registers[dest_reg] |= src_value;

    // Output the result
    char msg[BUFFER_SIZE];
    sprintf(msg, "R%d = %d\r\n", dest_reg, registers[dest_reg]);
    AddProgramMessage(msg);
}

/// @brief Perform bitwise XOR operation on two values and store the result in the destination register
/// @param dest_token Character array containing the destination token
/// @param src_token Character array containing the source token
void reg_xor(char *dest_token, char *src_token) {
    int dest_reg;
    int32_t src_value;

    // Parse operands
    if (!parse_operands(dest_token, src_token, &dest_reg, &src_value)) {
        return;
    }

    // Perform bitwise XOR operation
    registers[dest_reg] ^= src_value;

    // Output the result
    char msg[BUFFER_SIZE];
    sprintf(msg, "R%d = %d\r\n", dest_reg, registers[dest_reg]);
    AddProgramMessage(msg);
}



//==============================================================================
// Multiplication, Division, and Remainder
//==============================================================================

/// @brief Parses operands for arithmetic operations
/// @param dest_token Character array containing the destination token
/// @param src_token Character array containing the source token
/// @param dest_reg Integer to store the destination register
/// @param src_value 32-bit integer to store the source value
/// @return 
bool parse_operands(char *dest_token, char *src_token, int *dest_reg, int32_t *src_value) {
    uint32_t address;
    *dest_reg = parse_register(dest_token);
    if (*dest_reg == -1) {
        AddProgramMessage("Error: Invalid destination register.\r\n");  // TODO: Add to errors
        return false;
    }

    int src_reg = parse_register(src_token);
    if (src_reg != -1) {
        // Source is a register
        *src_value = registers[src_reg];
        AddProgramMessage("Register.\r\n");
    }
    else if (parse_immediate(src_token, src_value)) {
        // Immediate value
        AddProgramMessage("Immediate.\r\n");
    }
    else if (parse_memory_address(src_token, &address)) {
        if (!is_valid_memory_address(address)) {
            AddProgramMessage("Error: Invalid memory address.\r\n");  // TODO: Add to errors
            return false;
        }
        // Memory address (grad students)
        *src_value = *(int32_t *)address;
        AddProgramMessage("Memory Address.\r\n");
    }
    else {
        AddProgramMessage("Error: Invalid source operand.\r\n");  // TODO: Add to errors
        return false;
    }

    return true;
}

/// @brief Add two values and store the result in the destination register
/// @param dest_token 
/// @param src_token 
void reg_add(char *dest_token, char *src_token) {
    int dest_reg;
    int32_t src_value;

    if (!parse_operands(dest_token, src_token, &dest_reg, &src_value)) {
        return;
    }

    registers[dest_reg] += src_value;

    char msg[BUFFER_SIZE];
    sprintf(msg, "R%d = %d\r\n", dest_reg, registers[dest_reg]);
    AddProgramMessage(msg);
}

/// @brief Subtract two values and store the result in the destination register
/// @param dest_token 
/// @param src_token 
void reg_sub(char *dest_token, char *src_token) {
    int dest_reg;
    int32_t src_value;

    if (!parse_operands(dest_token, src_token, &dest_reg, &src_value)) {
        return;
    }

    registers[dest_reg] -= src_value;

    char msg[BUFFER_SIZE];
    sprintf(msg, "R%d = %d\r\n", dest_reg, registers[dest_reg]);
    AddProgramMessage(msg);
}

/// @brief Multiply two values and store the result in the destination register
/// @param dest_token 
/// @param src_token 
void reg_mul(char *dest_token, char *src_token) {
    int dest_reg;
    int32_t src_value;

    if (!parse_operands(dest_token, src_token, &dest_reg, &src_value)) {
        return;
    }

    registers[dest_reg] *= src_value;

    char msg[BUFFER_SIZE];
    sprintf(msg, "R%d = %d\r\n", dest_reg, registers[dest_reg]);
    AddProgramMessage(msg);
}

/// @brief Divide two values and store the result in the destination register
/// @param dest_token 
/// @param src_token 
void reg_div(char *dest_token, char *src_token) {
    int dest_reg;
    int32_t src_value;

    if (!parse_operands(dest_token, src_token, &dest_reg, &src_value)) {
        return;
    }

    if (src_value == 0) {
        AddProgramMessage("Error: Division by zero.\r\n");
        return;
    }

    registers[dest_reg] /= src_value;

    char msg[BUFFER_SIZE];
    sprintf(msg, "R%d = %d\r\n", dest_reg, registers[dest_reg]);
    AddProgramMessage(msg);
}

/// @brief Take the remainder of two values and store the result in the destination register
/// @param dest_token 
/// @param src_token 
void reg_rem(char *dest_token, char *src_token) {
    int dest_reg;
    int32_t src_value;

    if (!parse_operands(dest_token, src_token, &dest_reg, &src_value)) {
        return;
    }

    if (src_value == 0) {
        AddProgramMessage("Error: Division by zero.\r\n");
        return;
    }

    registers[dest_reg] %= src_value;

    char msg[BUFFER_SIZE];
    sprintf(msg, "R%d = %d\r\n", dest_reg, registers[dest_reg]);
    AddProgramMessage(msg);
}

/// @brief Negate the value in a register
/// @param reg_token 
void reg_neg(char *reg_token) {
    int reg = parse_register(reg_token);

    if (reg == -1) {
        AddProgramMessage("Error: Invalid register.\r\n");
        return;
    }

    registers[reg] = -registers[reg];

    char msg[BUFFER_SIZE];
    sprintf(msg, "R%d = %d\r\n", reg, registers[reg]);
    AddProgramMessage(msg);
}


//==============================================================================
// Maximum and Minimum
//==============================================================================

/// @brief Store the maximum of two values in the destination register
/// @param dest_token Character array containing the destination token
/// @param src_token Character array containing the source token
void reg_max(char *dest_token, char *src_token) {
    int dest_reg;
    int32_t src_value;

    // Parse operands
    if (!parse_operands(dest_token, src_token, &dest_reg, &src_value)) {
        return;
    }

    // Determine the maximum value
    if (registers[dest_reg] < src_value) {
        registers[dest_reg] = src_value;
    }

    // Output the result
    char msg[BUFFER_SIZE];
    sprintf(msg, "R%d = %d\r\n", dest_reg, registers[dest_reg]);
    AddProgramMessage(msg);
}

/// @brief Store the minimum of two values in the destination register
/// @param dest_token Character array containing the destination token
/// @param src_token Character array containing the source token
void reg_min(char *dest_token, char *src_token) {
    int dest_reg;
    int32_t src_value;

    // Parse operands
    if (!parse_operands(dest_token, src_token, &dest_reg, &src_value)) {
        return;
    }

    // Determine the minimum value
    if (registers[dest_reg] > src_value) {
        registers[dest_reg] = src_value;
    }

    // Output the result
    char msg[BUFFER_SIZE];
    sprintf(msg, "R%d = %d\r\n", dest_reg, registers[dest_reg]);
    AddProgramMessage(msg);
}
