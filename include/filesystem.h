/*
 * filesystem.h
 * ------------
 * A tiny file system backed by the Arduino's EEPROM.
 *
 * EEPROM layout:
 *   [ FAT: MAX_FILES x FATEntry ][ noOfFiles (1 byte) ][ file data ... ]
 *
 * The FAT records, per file, its name, start address and length. Files do not
 * need to be contiguous; free space is found by sorting the FAT on start
 * address and looking at the gaps between files.
 */

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <Arduino.h>
#include "config.h"

// One File Allocation Table entry. Stored to/read from EEPROM as a whole.
struct FATEntry {
  char name[FILENAMESIZE]; // file name, zero-terminated
  int start;               // start address of the data in EEPROM
  int size;                // length of the data in bytes
};

// ---- Setup ----------------------------------------------------------------
void fsBegin(); // initialise noOfFiles on first ever boot

// ---- Low-level EEPROM byte access (also used by running programs) ---------
byte readByteEEPROM(int address);
void writeByteEEPROM(int address, byte value);

// ---- FAT helpers ----------------------------------------------------------
int findFile(const char *name);          // FAT index, or -1 if not found
bool getFileInfo(const char *name, int &start, int &size); // lookup by name
int findFreeSpace(int size);             // start address of a gap >= size, or -1
int maxFreeSpace();                      // size of the largest free gap
bool createFile(const char *name, int size, int &startAddress); // allocate entry

// ---- Command-line commands -------------------------------------------------
void storeCommand();     // STORE name size <data bytes>
void retrieveCommand();  // RETRIEVE name
void eraseCommand();     // ERASE name
void filesCommand();     // FILES
void freespaceCommand(); // FREESPACE

#endif // FILESYSTEM_H
