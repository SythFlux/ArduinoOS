/*
 * main.cpp - ArduinOS entry point.
 *
 * setup() initialises the file system and the command line interface; loop()
 * services the CLI and advances all running processes once per iteration. The
 * process table is a global, so it starts zero-initialised: state 0 ==
 * TERMINATED means every slot is free at boot.
 */

#include <Arduino.h>
#include "cli.h"
#include "filesystem.h"
#include "process.h"

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(-1); // STORE keeps waiting for incoming data, never gives up
  fsBegin();             // format the EEPROM file system on first ever boot
  cliBegin();            // print "ArduinOS 1.0 ready"
}

void loop() {
  handleCLI();    // read a command token (if any) and run its handler
  runProcesses(); // give every running process one instruction
}
