#include <stdio.h>
#include <stdlib.h>

#include "include/emulator.h"

void testInstruction(Emulator *emulator) {
  emulator->program_counter = ROM_START_ADDR;
  writeIns(emulator, 0x2400); // Changed from 0x2398 to 0x2400
  writeIns(emulator, 0x0FFF); // custom print register ins
  writeIns(emulator, 0x1204);
  
  emulator->program_counter = 0x400;
  writeIns(emulator, 0x0FFF);
  writeIns(emulator, 0x00EE);
  
  emulator->program_counter = ROM_START_ADDR;
}

int main(int argc, char **argv) {
  Emulator *emulator = createEmulator();
  testInstruction(emulator);
  start(emulator);
  //dumpMemory(emulator);
  //dumpRegister(emulator);
  freeEmulator(emulator);
  return 0;
} 

