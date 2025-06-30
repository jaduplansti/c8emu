#ifndef _EMULATOR_H_
#define _EMULATOR_H_

#include <stdlib.h>
#include <stdint.h>

#define MEM_SIZE 4096 // 4kb, MEM_SIZE is in bytes.
#define TOTAL_REGISTERS 16 // there are 16 registers that are 8 bit in chip 8.
#define ROM_START_ADDR 0x200 // the starting address if roms (probably)
#define STACK_SIZE 16 // max stack size
#define INSTRUCTION_PER_SEC 700.0f


typedef struct { // structure to hold emulator shit.
  uint8_t memory[MEM_SIZE];
  uint8_t registers[TOTAL_REGISTERS];
  uint16_t stack[STACK_SIZE];
  
  uint16_t index_register;
  uint16_t program_counter;
  uint8_t carry_flag;
  
  uint8_t stack_pointer;
} Emulator;

Emulator *createEmulator(); 
void freeEmulator(Emulator *emulator);

uint8_t readRegister(Emulator *emulator, size_t index);
void writeRegister(Emulator *emulator, size_t index, uint8_t value);
void dumpRegister(Emulator *emulator);

void throwError(Emulator *emulator, const char *msg);
void loadRom(Emulator *emulator, const char *file);

void dumpMemory(Emulator *emulator);
void writeMemory(Emulator *emulator, uint16_t address, uint8_t byte);
void writeIns(Emulator *emulator, uint16_t opcode);

void start(Emulator *emulator);
uint16_t fetch(Emulator *emulator);
uint16_t getNibble(Emulator *emulator, uint16_t opcode, size_t nibble);

void pushStack(Emulator *emulator, uint16_t value);
uint16_t popStack(Emulator *emulator);
void execute(Emulator *emulator, uint16_t opcode);
#endif