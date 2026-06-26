// A small EEPROM file system.
// Layout: [ FAT: MAX_FILES x FATEntry ][ noOfFiles (1 byte) ][ file data... ]
// Files needn't be contiguous; free space is found from the gaps in the FAT.

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <Arduino.h>
#include "config.h"

struct FATEntry {
  char name[FILENAMESIZE];
  int start; // start address of the data in EEPROM
  int size;  // length in bytes
};

void fsBegin();

byte readByteEEPROM(int address);
void writeByteEEPROM(int address, byte value);

int findFile(const char *name);                             // FAT index, or -1
bool getFileInfo(const char *name, int &start, int &size);
int findFreeSpace(int size);                                // start of a gap, or -1
int maxFreeSpace();
bool createFile(const char *name, int size, int &startAddress);

void storeCommand();     // STORE name size <data bytes>
void retrieveCommand();  // RETRIEVE name
void eraseCommand();     // ERASE name
void filesCommand();     // FILES
void freespaceCommand(); // FREESPACE

#endif // FILESYSTEM_H
