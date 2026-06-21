/*
 * process.h
 * ---------
 * The process table and process life-cycle. Each process runs a bytecode file
 * and owns its own registers (PC, FP, SP, loop register) and stack. Processes
 * can be started, paused, resumed and killed; runProcesses() advances every
 * RUNNING process by one instruction.
 */

#ifndef PROCESS_H
#define PROCESS_H

#include <Arduino.h>
#include "config.h"

// One entry in the process table.
struct ProcessType {
  char name[FILENAMESIZE]; // program / file name
  int pid;                 // unique process id
  char state;              // RUNNING / PAUSED / TERMINATED
  int pc;                  // program counter (absolute EEPROM address)
  int fp;                  // file pointer  (absolute EEPROM address)
  byte sp;                 // stack pointer (index into stack[])
  int loopReg;             // PC saved by LOOP, restored by ENDLOOP
  byte stack[STACKSIZE];   // this process's private stack
};

// The process table is shared with the instruction interpreter.
extern ProcessType processTable[MAX_PROCESSES];

// ---- Life cycle -----------------------------------------------------------
// Starts file `name` as a new process. Returns the new pid, or -1 on failure
// (no free table slot, or file not found). Used by RUN and the FORK opcode.
int startProcess(const char *name);

// Returns the table index of the (non-terminated) process with id `pid`, or -1.
int findProcessByPid(int pid);

// Advances every RUNNING process by one instruction. Defined in instructions
// .cpp because it drives the interpreter; declared here as it is process work.
void runProcesses();

// ---- Command-line commands -------------------------------------------------
void runCommand();     // RUN name
void listCommand();    // LIST
void suspendCommand(); // SUSPEND id
void resumeCommand();  // RESUME id
void killCommand();    // KILL id

#endif // PROCESS_H
