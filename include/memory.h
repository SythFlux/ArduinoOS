#ifndef MEMORY_H
#define MEMORY_H

#include <Arduino.h>
#include "config.h"

struct MemEntry {
  byte name;    // 1-character variable name
  byte type;    // CHAR / INT / FLOAT / STRING
  byte address; // start address in the memory array
  byte size;    // number of bytes used
  int processId;
};

void storeVariable(byte name, int processId);
void readVariable(byte name, int processId);
void clearProcessVariables(int processId);   // called when a process ends
byte countProcessVariables(int processId);

#endif // MEMORY_H
