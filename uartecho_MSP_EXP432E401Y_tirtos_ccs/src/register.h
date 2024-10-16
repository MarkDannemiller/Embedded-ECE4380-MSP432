// register.h
#ifndef REG_H
#define REG_H

#include <stdint.h>

#define NUM_REGISTERS 32

extern int32_t registers[NUM_REGISTERS];


void init_registers();
void print_all_registers();

int parse_register(char *token);
bool parse_immediate(char *token, int32_t *value);
bool parse_memory_address(char *token, uint32_t *address);
bool is_valid_memory_address(uint32_t address);

// Register operations
void reg_mov(char *dest_token, char *src_token);
void reg_xchg(char *reg1_token, char *reg2_token);
void reg_inc(char *reg_token);
void reg_dec(char *reg_token);

// Logical operations
void reg_not(char *reg_token);
void reg_and(char *dest_token, char *src_token);
void reg_ior(char *dest_token, char *src_token);
void reg_xor(char *dest_token, char *src_token);

// Multiplication, Division, and Remainder
bool parse_operands(char *dest_token, char *src_token, int *dest_reg, int32_t *src_value);
void reg_add(char *dest_token, char *src_token);
void reg_sub(char *dest_token, char *src_token);
void reg_mul(char *dest_token, char *src_token);
void reg_div(char *dest_token, char *src_token);
void reg_rem(char *dest_token, char *src_token);
void reg_neg(char *reg_token);

// Maximum and Minimum
void reg_max(char *dest_token, char *src_token);
void reg_min(char *dest_token, char *src_token);

#endif // REG_H
