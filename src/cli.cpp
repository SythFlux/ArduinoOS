/*
 * cli.cpp - command line interface implementation.
 */

#include <Arduino.h>
#include "cli.h"
#include "config.h"
#include "filesystem.h"
#include "process.h"

// A command name and the function that handles it.
typedef struct {
  const char *name;
  void (*func)();
} commandType;

// All available commands (see table 1 in the manual). Adding a command is just
// one line here plus its handler function.
static commandType command[] = {
  { "store",     &storeCommand },
  { "retrieve",  &retrieveCommand },
  { "erase",     &eraseCommand },
  { "files",     &filesCommand },
  { "freespace", &freespaceCommand },
  { "run",       &runCommand },
  { "list",      &listCommand },
  { "suspend",   &suspendCommand },
  { "resume",    &resumeCommand },
  { "kill",      &killCommand },
};
static const int noOfCommands = sizeof(command) / sizeof(commandType);

// ---------------------------------------------------------------------------
// Non-blocking token reader
// ---------------------------------------------------------------------------
// The buffer is static so it survives across loop() iterations: a token may be
// typed one character at a time over many iterations.
static char buffer[BUFSIZE];
static byte pos = 0;

// Returns true exactly once, when a complete token is available in `buffer`.
static bool readToken() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == ' ' || c == '\n' || c == '\r') {
      if (pos == 0) continue;     // ignore separators before a token starts
      buffer[pos] = '\0';
      pos = 0;
      return true;                // token complete
    }
    if (pos < BUFSIZE - 1) buffer[pos++] = c; // store, guard against overflow
  }
  return false;                   // no complete token yet
}

void waitForToken(char *dest) {
  while (!readToken()) runProcesses(); // keep processes alive while we wait
  strcpy(dest, buffer);
}

// ---------------------------------------------------------------------------
// Command dispatch
// ---------------------------------------------------------------------------
static void dispatch(const char *name) {
  for (int i = 0; i < noOfCommands; i++) {
    if (strcasecmp(name, command[i].name) == 0) {
      command[i].func();
      return;
    }
  }
  // Unknown command: report it and list what is available.
  Serial.print(F("Unknown command: "));
  Serial.println(name);
  Serial.print(F("Available:"));
  for (int i = 0; i < noOfCommands; i++) {
    Serial.print(' ');
    Serial.print(command[i].name);
  }
  Serial.println();
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------
void cliBegin() {
  Serial.println(F("ArduinOS 1.0 ready"));
}

void handleCLI() {
  if (readToken()) dispatch(buffer);
}
