#ifndef PROCESS_H
#define PROCESS_H

#include <Arduino.h>
#include "config.h"

struct ProcessType {
  char name[FILENAMESIZE]; // program / file name
  int pid;
  char state;              // RUNNING / PAUSED / TERMINATED
  int pc;                  // program counter (EEPROM address)
  int fp;                  // file pointer (EEPROM address)
  byte sp;                 // stack pointer (index into stack[])
  int loopReg;             // PC saved by LOOP, restored by ENDLOOP
  byte stack[STACKSIZE];
};

extern ProcessType processTable[MAX_PROCESSES];

int startProcess(const char *name);
int findProcessByPid(int pid); // table index, or -1
void runProcesses();           // defined in instructions.cpp

void runCommand();     // RUN name
void listCommand();    // LIST
void suspendCommand(); // SUSPEND id
void resumeCommand();  // RESUME id
void killCommand();    // KILL id

#endif // PROCESS_H
