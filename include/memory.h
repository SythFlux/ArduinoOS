/*
 * memory.h
 * --------
 * Working memory + a memory table for variables. Variables are stored in a
 * 256-byte RAM array; the memory table records where each variable lives.
 *
 * Variables are moved between this memory and a process's stack:
 *   - storeVariable() pops a value off the stack into memory.
 *   - readVariable()  pushes a variable from memory onto the stack.
 * A variable is uniquely identified by (name, processId), so two processes may
 * use the same variable name independently.
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <Arduino.h>
#include "config.h"

// One memory-table entry describing a stored variable.
struct MemEntry {
  byte name;    // 1-character variable name
  byte type;    // CHAR / INT / FLOAT / STRING
  byte address; // start address within the 256-byte memory array
  byte size;    // number of bytes used
  int processId;// owning process
};

// Pops a value (incl. type) off the selected stack and stores it as variable
// `name` for `processId`. Overwrites an existing variable with the same
// name+process. Prints an error if the table or memory is full.
void storeVariable(byte name, int processId);

// Pushes variable `name` of `processId` onto the selected stack (value, plus
// length for strings, plus type). Prints an error if it does not exist.
void readVariable(byte name, int processId);

// Frees every variable belonging to `processId` (called when a process ends).
void clearProcessVariables(int processId);

#endif // MEMORY_H
