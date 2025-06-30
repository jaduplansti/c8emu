#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/assembler.h"

Assembler *createAssembler() {
  Assembler *assembler = calloc(1, sizeof(Assembler));
  return assembler;
}

void freeAssembler(Assembler *assembler) {
  free(assembler);
}

void throwError(Assembler *assembler, const char *msg) {
  free(assembler);
  printf("%s\n", msg);
  exit(EXIT_FAILURE);
}

void openFile(Assembler *assembler, char *file_name) {
  assembler->file = fopen(file_name, "r");
  if (assembler->file == NULL) throwError(assembler, "file does not exist!");
}

void assemble(Assembler *assembler) {

}
