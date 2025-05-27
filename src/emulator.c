#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <time.h>
#include "include/emulator.h"

Emulator *createEmulator() {
  srand(time(NULL));
  Emulator *emulator = calloc(1, sizeof(Emulator));
  emulator->stack_pointer = 0;
  return emulator;
}

void freeEmulator(Emulator *emulator) {
  free(emulator);
}

uint8_t readRegister(Emulator *emulator, size_t index) {
  if (index >= TOTAL_REGISTERS) throwError(emulator, "failed to read, invalid register index.");
  return emulator->registers[index];
}

void writeRegister(Emulator *emulator, size_t index, uint8_t value) {
  if (index >= TOTAL_REGISTERS) throwError(emulator, "failed to write, invalid register index.");
  emulator->registers[index] = value;
}

void dumpRegister(Emulator *emulator) {
  printf("====REGISTERS====\n\n");
  for (size_t n = 0; n < TOTAL_REGISTERS; n++) {
    printf("V%d = %04x\n", (int) n, readRegister(emulator, n));
  }
  printf("\n===============\n\n");
}

void throwError(Emulator *emulator, const char *msg) {
  printf("[error]: %s\n", msg);
  freeEmulator(emulator);
  exit(-1);
}

void loadRom(Emulator *emulator, const char *file) {
  FILE *rom = fopen(file, "rb");
  if (rom == NULL) throwError(emulator, "failed to load rom in memory, are you sure the file exists?");
 
  fseek(rom, 0, SEEK_END);
  size_t rom_size = ftell(rom);
  fseek(rom, 0, SEEK_SET);
  
  if (rom_size > MEM_SIZE - ROM_START_ADDR) {
    fclose(rom);
    throwError(emulator, "failed to rom in memory, rom should be under MEM_SIZE (4096) bytes");
  }
  
  uint8_t *bytes = calloc(rom_size + 1, sizeof(uint8_t));
  if(fread(bytes, 1, rom_size, rom) != rom_size) {
    fclose(rom);
    throwError(emulator, "failed to fully load rom.");
  }
  
  
  memcpy(&emulator->memory[ROM_START_ADDR], bytes, rom_size);
  emulator->program_counter = ROM_START_ADDR;
  free(bytes);
  fclose(rom);
}

void dumpMemory(Emulator *emulator) {
  printf("=====MEMORY=====\n\n");
  for(size_t n = 0; n < MEM_SIZE; n++) {
    if (n % 8 == 0 && n != 0) printf("\x1b[32m 0x%0x\x1b[0m\n", (unsigned int) n);
    printf("%02x", emulator->memory[n]);
  }
  printf("\n\n===============\n\n");
}

uint16_t fetch(Emulator *emulator) {
  uint8_t high = emulator->memory[emulator->program_counter];
  uint8_t low = emulator->memory[emulator->program_counter + 1];
  
  emulator->program_counter += 2;
  return (high << 8) | low;
}

uint16_t getNibble(Emulator *emulator, uint16_t opcode, size_t nibble) {
  switch(nibble) {
    case 1:
      return (opcode & 0xF000) >> 12;
    break;
    
    case 2:
      return (opcode & 0x0F00) >> 8;
    break; 
    
    case 3:
      return (opcode & 0x00F0) >> 4;
    break;
    
    case 4:
      return (opcode & 0x000F);
    break; 
    
    case 5:
      return (opcode & 0x0FFF);
    break;
  }
  
  throwError(emulator, "failed to get nibble.");
  return -1; // compiler wont shut up lol, unreachable.
}

void writeMemory(Emulator *emulator, uint16_t address, uint8_t byte) {
  if (address >= MEM_SIZE) throwError(emulator, "failed to write to memory, address > MEM_SIZE (4096)");
  emulator->memory[address] = byte;
}

void writeIns(Emulator *emulator, uint16_t opcode) {
  uint8_t high = (opcode >> 8) & 0xFF;
  uint8_t low = opcode & 0xFF;
  
  writeMemory(emulator, emulator->program_counter, high);
  writeMemory(emulator, emulator->program_counter + 1, low);

  emulator->program_counter += 2;
}

void pushStack(Emulator *emulator, uint16_t value) {
  if (emulator->stack_pointer >= STACK_SIZE) {
    throwError(emulator, "failed to push value onto stack, (stack overflow) [value >= 16]");
  }
  emulator->stack[emulator->stack_pointer] = value;
  emulator->stack_pointer += 1;
}

uint16_t popStack(Emulator *emulator) {
  if (emulator->stack_pointer == 0) throwError(emulator, "failed to pop stack, maybe because that stack has nothing (sp <= 0)");
  return emulator->stack[--emulator->stack_pointer];
}

void start(Emulator *emulator) {
  while(emulator->program_counter < MEM_SIZE) {
    uint16_t opcode = fetch(emulator);
    execute(emulator, opcode);
  }
}

void execute(Emulator *emulator, uint16_t opcode) {
  uint16_t nibble1 = getNibble(emulator, opcode, 1);
  uint16_t nibble2 = getNibble(emulator, opcode, 2);
  uint16_t nibble3 = getNibble(emulator, opcode, 3);
  uint16_t nibble4 = getNibble(emulator, opcode, 4);
  
  switch(nibble1) {
    case 0x0: {
      if (opcode == 0x00E0) throwError(emulator, "clear instruction not implemented, halting..");
      else if(opcode == 0x00EE) {
        uint16_t address = popStack(emulator);
        emulator->program_counter = address;
      }
      else if(opcode == 0x0FFF) dumpRegister(emulator);
      break;
    }
    
    case 0x1: { // 0xnnn, (jmp nnn) jumps to nnn
      uint16_t address = opcode & 0x0FFF;
      if (address == emulator->program_counter - 2) address = MEM_SIZE;
      emulator->program_counter = address;
      break;
    }
    
    case 0x2: { // 0x2nnn, (call nnn) calls the subroutine nnn, saves the stack pointer
      pushStack(emulator, emulator->program_counter);
      emulator->program_counter = opcode & 0x0FFF;
      break;
    }
    
    case 0x3: { // 0x3xkk (skip if Vx = kk) program counter jumps by 2 if register Vx = kk (byte)
      uint8_t byte = opcode & 0x00FF;
      if(readRegister(emulator, nibble2) == byte) emulator->program_counter += 2;
      break;
    }
    
    case 0x4: { // 0x4xkk (skip if Vx != kk) program counter jumps by 2 if register Vx != kk (byte)
      uint8_t byte = opcode & 0x00FF;
      if(readRegister(emulator, nibble2) != byte) emulator->program_counter += 2;
      break;
    }
    
    case 0x5: { // 0x5xy0 (skip if Vx = Vy) program counter jumps by 2 if register Vx = register Vy
      if(readRegister(emulator, nibble2) == readRegister(emulator, nibble3)) emulator->program_counter += 2;
      break;
    }
    
    case 0x6: { // 0x6xkk (set Vx = kk) sets the register Vx to kk (byte)
      uint8_t byte = opcode & 0x00FF;
      writeRegister(emulator, nibble2, byte);
      break;
    }
    
    case 0x7: { // 0x7xkk (add Vx = Vx + kk) adds kk (byte) to the register Vx 
      uint8_t byte = opcode & 0x00FF;
      uint8_t new_value = readRegister(emulator, nibble2) + byte;
      writeRegister(emulator, nibble2, new_value);
      break;
    }
   
   case 0x8: { // 0x8xyx register operations
    if (nibble4 == 0) { // 0x8xy0 (set Vx = Vy) sets the register Vx to the register Vy
      uint8_t vy = readRegister(emulator, nibble3);
      writeRegister(emulator, nibble2, vy);     
     }
     
    else if (nibble4 == 1) { // 0x8xy1 (set Vx = Vx | Vy) sets the register Vx to Vx OR Vy
      uint8_t vy = readRegister(emulator, nibble3);
      uint8_t vx = readRegister(emulator, nibble2);
      writeRegister(emulator, nibble2, vx | vy);     
    }
     
    else if (nibble4 == 2) { // 0x8xy2 (set Vx = Vx & Vy) sets the register Vx to Vx AND Vy
      uint8_t vy = readRegister(emulator, nibble3);
      uint8_t vx = readRegister(emulator, nibble2);
      writeRegister(emulator, nibble2, vx & vy);     
    }
    
    else if (nibble4 == 3) { // 0x8xy3 (set Vx = Vx ^ Vy) sets the register Vx to Vx XOR Vy
      uint8_t vy = readRegister(emulator, nibble3);
      uint8_t vx = readRegister(emulator, nibble2);
      writeRegister(emulator, nibble2, vx ^ vy);     
    }
    
    else if (nibble4 == 4) { // 0x8xy4 (set Vx = Vx + Vy, VF = carry) sets the register Vx to Vx + Vy, sets VF if carry
      uint8_t vy = readRegister(emulator, nibble3);
      uint8_t vx = readRegister(emulator, nibble2);
      uint16_t sum = (uint16_t) vx + (uint16_t) vy;
      writeRegister(emulator, nibble2, (uint8_t) sum);     
       
      if (sum > 255) emulator->carry_flag = 1;
      else emulator->carry_flag = 0;
    }
    
    else if (nibble4 == 5) { // 0x8xy5 (set Vx = Vx - Vy, VF = not borrow) sets the register Vx to Vx - Vy, sets VF if not borrow
      uint8_t vy = readRegister(emulator, nibble3);
      uint8_t vx = readRegister(emulator, nibble2);
      writeRegister(emulator, nibble2, vx - vy);     
       
      if (vx > vy) emulator->carry_flag = 1;
      else emulator->carry_flag = 0;
    }
   
    else if (nibble4 == 6) { // 0x8xy6 (set Vx = Vx >> 1, VF = significant) sets the register Vx to Vx >> 1, sets VF if significant
      uint8_t vx = readRegister(emulator, nibble2);
      writeRegister(emulator, nibble2, vx >> 1);     
       
      if ((vx & 0x01) == 1) emulator->carry_flag = 1;
      else emulator->carry_flag = 0;
    }
   
    else if (nibble4 == 7) { // 0x8xy7 (set Vx = Vy - Vx, VF = not borrow) sets the register Vx to Vy - Vx, sets VF if not borrow
      uint8_t vy = readRegister(emulator, nibble3);
      uint8_t vx = readRegister(emulator, nibble2);
      writeRegister(emulator, nibble2, vy - vx);     
       
      if (vy > vx) emulator->carry_flag = 1;
      else emulator->carry_flag = 0;
    }
    
    else if (nibble4 == 0xE) { // 0x8xyE (set Vx = Vx << 1, VF = significant) sets the register Vx to Vx << 1, sets VF if significant
      uint8_t vx = readRegister(emulator, nibble2);
      writeRegister(emulator, nibble2, vx << 1);     
       
      if ((vx & 0x80) == 0x80) emulator->carry_flag = 1;
      else emulator->carry_flag = 0;
    }
    break;
  }
  
  case 0x9: { // 0x9xy0 (skip if Vx != Vy) program counter jumps by 2 if register Vx != register Vy
    if(readRegister(emulator, nibble2) != readRegister(emulator, nibble3)) emulator->program_counter += 2;
    break;
  }
  
  case 0xA: { // 0xAnnn (I = nnn) sets the index register to nnn (address)
    emulator->index_register = opcode & 0x0FFF;
    break;
  }
  
  case 0xB: { // 0xBnnn (jmp [nnn + V0]), jumps to nnn + register V0
    emulator->program_counter = (opcode & 0x0FFF) + readRegister(emulator, 0);
    break;
  }
  
  case 0xC: { // 0xCxkk (set Vx = RND & kk) sets the register Vx to RND (random byte) AND kk (byte)
    uint8_t rnd = rand() % 256;
    writeRegister(emulator, nibble2, rnd & (opcode & 0x00FF));
    break;
  }
  
  case 0xD: { // 0xDxyn (draw Vx, Vy, Nibble) draws the nibble to the screen (I) with the coordinates stored at register Vx (x axis) and register Vy (y axis)
    throwError(emulator, "instruction unimplemented, halting..");
    break;
  }
  
  case 0xE: { // 0xExxx (keyboard controls)
    if ((opcode & 0x00FF) == 0x9E) { // 0xEx9E (skip Vx) skips next instruction if Keyboard Input = Vx
      throwError(emulator, "instruction unimplemented, halting..");
    }
    else if ((opcode & 0x00FF) == 0xA1) { // 0xEx9E (skip not Vx) skips next instruction if Keyboard Input != Vx
      throwError(emulator, "instruction unimplemented, halting..");
    }
    break;
  }
 
  case 0xF: { // 0xFxxx (timer operations)
   throwError(emulator, "timer unimplemented, halting..");
   break;
  }
  
  default: throwError(emulator, "unknown instruction, [0-F], how the hell did you arrive here.");
 }
}