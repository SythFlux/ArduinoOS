#include <Arduino.h>
#include "cli.h"
#include "config.h"
#include "filesystem.h"
#include "process.h"

typedef struct {
  const char *name;
  void (*func)();
} commandType;

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

// Buffer is static because a token can arrive one character per loop() call.
static char buffer[BUFSIZE];
static byte pos = 0;

// True once when a full token is ready in buffer.
static bool readToken() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == ' ' || c == '\n' || c == '\r') {
      if (pos == 0) continue;
      buffer[pos] = '\0';
      pos = 0;
      return true;
    }
    if (pos < BUFSIZE - 1) buffer[pos++] = c;
  }
  return false;
}

void waitForToken(char *dest) {
  while (!readToken()) runProcesses(); // keep processes running while we wait
  strcpy(dest, buffer);
}

static void dispatch(const char *name) {
  for (int i = 0; i < noOfCommands; i++) {
    if (strcasecmp(name, command[i].name) == 0) {
      command[i].func();
      return;
    }
  }
  Serial.print(F("Unknown command: "));
  Serial.println(name);
  Serial.print(F("Available:"));
  for (int i = 0; i < noOfCommands; i++) {
    Serial.print(' ');
    Serial.print(command[i].name);
  }
  Serial.println();
}

void cliBegin() {
  Serial.println(F("DonnyOS Begin"));
}

void handleCLI() {
  if (readToken()) dispatch(buffer);
}
