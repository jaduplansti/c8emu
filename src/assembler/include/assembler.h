#ifndef _ASSEMBLER_H_
#define _ASSEMBLER_H_

#include <stdio.h>

typedef struct {
  FILE *file;
} Assembler;

Assembler *createAssembler();
void freeAssembler(Assembler *assembler);
void throwError(Assembler *assembler, const char *msg);

void openFile(Assembler *assembler, char *file_name);
void assemble(Assembler *assembler);

#endif