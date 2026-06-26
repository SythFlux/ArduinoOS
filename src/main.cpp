#include <Arduino.h>
#include "cli.h"
#include "filesystem.h"
#include "process.h"

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(-1); // STORE waits indefinitely for incoming data
  fsBegin();
  cliBegin();
}

void loop() {
  handleCLI();    // read a command token (if any) and run it
  runProcesses(); // advance every running process one instruction
}
