/*
 * process.cpp - process table and life-cycle management.
 *
 * (runProcesses() lives in instructions.cpp because it drives the bytecode
 *  interpreter; everything else about processes is here.)
 */

#include "process.h"
#include "filesystem.h"
#include "memory.h"
#include "cli.h" // waitForToken()

ProcessType processTable[MAX_PROCESSES];
static int nextPid = 1; // ever-increasing id counter

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
int findProcessByPid(int pid) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (processTable[i].state != TERMINATED && processTable[i].pid == pid) return i;
  }
  return -1;
}

// First free (TERMINATED) slot, or -1 if the table is full.
static int findFreeSlot() {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (processTable[i].state == TERMINATED) return i;
  }
  return -1;
}

// ---------------------------------------------------------------------------
// Starting a process (shared by RUN and FORK)
// ---------------------------------------------------------------------------
int startProcess(const char *name) {
  int slot = findFreeSlot();
  if (slot < 0) return -1; // process table full

  int start, size;
  if (!getFileInfo(name, start, size)) return -1; // no such file

  ProcessType &p = processTable[slot];
  strncpy(p.name, name, FILENAMESIZE);
  p.name[FILENAMESIZE - 1] = '\0';
  p.pid = nextPid++;
  p.state = RUNNING;
  p.pc = start; // program counter starts at the file's first byte
  p.fp = start; // file pointer defaults to the same place
  p.sp = 0;
  p.loopReg = 0;
  return p.pid;
}

// ---------------------------------------------------------------------------
// State changes (used by SUSPEND / RESUME / KILL)
// ---------------------------------------------------------------------------
static void setState(int index, char newState) {
  ProcessType &p = processTable[index];
  if (p.state == newState) {
    Serial.println(F("ERROR: process already in that state"));
    return;
  }
  p.state = newState;
  Serial.print(F("Process "));
  Serial.print(p.pid);
  Serial.print(F(" is now "));
  switch (newState) {
    case RUNNING:    Serial.println(F("running"));    break;
    case PAUSED:     Serial.println(F("paused"));      break;
    case TERMINATED: Serial.println(F("terminated"));  break;
  }
}

// Reads an id argument and returns its table index, printing an error if the
// process does not exist.
static int readProcessArg() {
  char idStr[BUFSIZE];
  waitForToken(idStr);
  int index = findProcessByPid(atoi(idStr));
  if (index < 0) Serial.println(F("ERROR: no such process"));
  return index;
}

// ---------------------------------------------------------------------------
// Command-line commands
// ---------------------------------------------------------------------------
void runCommand() {
  char name[BUFSIZE];
  waitForToken(name);
  int pid = startProcess(name);
  if (pid < 0) {
    Serial.println(F("ERROR: cannot start process (no slot or file missing)"));
    return;
  }
  Serial.print(F("Started process "));
  Serial.println(pid);
}

void listCommand() {
  Serial.println(F("id  state  pc    vars  name"));
  for (int i = 0; i < MAX_PROCESSES; i++) {
    ProcessType &p = processTable[i];
    if (p.state == TERMINATED) continue;
    Serial.print(p.pid);
    Serial.print(F("   "));
    Serial.print(p.state == RUNNING ? F("run  ") : F("pause"));
    Serial.print(F("  "));
    Serial.print(p.pc);              // program counter of this process
    Serial.print(F("   "));
    Serial.print(countProcessVariables(p.pid)); // variables owned by this process
    Serial.print(F("     "));
    Serial.println(p.name);
  }
}

void suspendCommand() {
  int index = readProcessArg();
  if (index >= 0) setState(index, PAUSED);
}

void resumeCommand() {
  int index = readProcessArg();
  if (index >= 0) setState(index, RUNNING);
}

void killCommand() {
  int index = readProcessArg();
  if (index < 0) return;
  clearProcessVariables(processTable[index].pid); // free its variables
  setState(index, TERMINATED);
}
