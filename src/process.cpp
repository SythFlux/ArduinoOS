// Process table and life-cycle. runProcesses() is in instructions.cpp.

#include "process.h"
#include "filesystem.h"
#include "memory.h"
#include "cli.h"

ProcessType processTable[MAX_PROCESSES];
static int nextPid = 1;

int findProcessByPid(int pid) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (processTable[i].state != TERMINATED && processTable[i].pid == pid) return i;
  }
  return -1;
}

static int findFreeSlot() {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (processTable[i].state == TERMINATED) return i;
  }
  return -1;
}

// Used by RUN and FORK.
int startProcess(const char *name) {
  int slot = findFreeSlot();
  if (slot < 0) return -1;

  int start, size;
  if (!getFileInfo(name, start, size)) return -1;

  ProcessType &p = processTable[slot];
  strncpy(p.name, name, FILENAMESIZE);
  p.name[FILENAMESIZE - 1] = '\0';
  p.pid = nextPid++;
  p.state = RUNNING;
  p.pc = start;
  p.fp = start;
  p.sp = 0;
  p.loopReg = 0;
  return p.pid;
}

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

// Reads an id argument and returns its table index, or -1 with an error.
static int readProcessArg() {
  char idStr[BUFSIZE];
  waitForToken(idStr);
  int index = findProcessByPid(atoi(idStr));
  if (index < 0) Serial.println(F("ERROR: no such process"));
  return index;
}

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
    Serial.print(p.pc);
    Serial.print(F("   "));
    Serial.print(countProcessVariables(p.pid));
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
  clearProcessVariables(processTable[index].pid);
  setState(index, TERMINATED);
}
