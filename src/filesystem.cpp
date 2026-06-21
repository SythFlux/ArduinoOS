/*
 * filesystem.cpp - EEPROM-backed file system implementation.
 */

#include <EEPROM.h>
#include "filesystem.h"
#include "cli.h"     // waitForToken(), runWhileWaiting helpers
#include "process.h" // runProcesses() so processes keep ticking during STORE

// ---------------------------------------------------------------------------
// EEPROM layout constants
// ---------------------------------------------------------------------------
static const int NO_OF_FILES_ADDR = MAX_FILES * sizeof(FATEntry);
static const int FS_MAGIC_ADDR = NO_OF_FILES_ADDR + 1;
static const int DATA_START = NO_OF_FILES_ADDR + 2; // first byte usable for data
static const byte FS_MAGIC = 0x42;                  // "file system initialised"

// noOfFiles lives in EEPROM and survives power loss; EERef lets us use it like
// a normal one-byte variable.
static EERef noOfFiles = EEPROM[NO_OF_FILES_ADDR];

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void fsBegin() {
  // On a brand-new chip EEPROM is filled with 0xFF; detect that with a magic
  // byte and format the file system the first time only.
  if (EEPROM.read(FS_MAGIC_ADDR) != FS_MAGIC) {
    noOfFiles = 0;
    EEPROM.update(FS_MAGIC_ADDR, FS_MAGIC);
  }
}

// ---------------------------------------------------------------------------
// Low-level EEPROM access
// ---------------------------------------------------------------------------
byte readByteEEPROM(int address) {
  return EEPROM.read(address);
}

void writeByteEEPROM(int address, byte value) {
  EEPROM.update(address, value); // update() skips the write if unchanged
}

// ---------------------------------------------------------------------------
// FAT entry read / write
// ---------------------------------------------------------------------------
static void readFATEntry(int index, FATEntry &entry) {
  EEPROM.get(index * sizeof(FATEntry), entry);
}

static void writeFATEntry(int index, const FATEntry &entry) {
  EEPROM.put(index * sizeof(FATEntry), entry);
}

// ---------------------------------------------------------------------------
// FAT helpers
// ---------------------------------------------------------------------------
int findFile(const char *name) {
  FATEntry entry;
  for (int i = 0; i < noOfFiles; i++) {
    readFATEntry(i, entry);
    if (strcmp(entry.name, name) == 0) return i;
  }
  return -1;
}

bool getFileInfo(const char *name, int &start, int &size) {
  int index = findFile(name);
  if (index < 0) return false;
  FATEntry entry;
  readFATEntry(index, entry);
  start = entry.start;
  size = entry.size;
  return true;
}

// Copy the FAT into `out` and sort it ascending by start address. Returns the
// number of entries. Used by the free-space algorithms below.
static int sortedFAT(FATEntry out[]) {
  int n = noOfFiles;
  for (int i = 0; i < n; i++) readFATEntry(i, out[i]);
  for (int i = 1; i < n; i++) { // simple insertion sort (n is small)
    FATEntry key = out[i];
    int j = i - 1;
    while (j >= 0 && out[j].start > key.start) {
      out[j + 1] = out[j];
      j--;
    }
    out[j + 1] = key;
  }
  return n;
}

int findFreeSpace(int size) {
  FATEntry fat[MAX_FILES];
  int n = sortedFAT(fat);
  int prevEnd = DATA_START;
  for (int i = 0; i < n; i++) {
    if (fat[i].start - prevEnd >= size) return prevEnd; // gap before this file
    prevEnd = fat[i].start + fat[i].size;
  }
  if ((int)EEPROM.length() - prevEnd >= size) return prevEnd; // gap at the end
  return -1;
}

int maxFreeSpace() {
  FATEntry fat[MAX_FILES];
  int n = sortedFAT(fat);
  int prevEnd = DATA_START;
  int best = 0;
  for (int i = 0; i < n; i++) {
    int gap = fat[i].start - prevEnd;
    if (gap > best) best = gap;
    prevEnd = fat[i].start + fat[i].size;
  }
  int gap = (int)EEPROM.length() - prevEnd;
  if (gap > best) best = gap;
  return best;
}

bool createFile(const char *name, int size, int &startAddress) {
  if (noOfFiles >= MAX_FILES) return false; // FAT full
  startAddress = findFreeSpace(size);
  if (startAddress < 0) return false;       // no room on "disk"

  FATEntry entry;
  strncpy(entry.name, name, FILENAMESIZE);
  entry.name[FILENAMESIZE - 1] = '\0';
  entry.start = startAddress;
  entry.size = size;
  writeFATEntry(noOfFiles, entry);
  noOfFiles = noOfFiles + 1;
  return true;
}

// ---------------------------------------------------------------------------
// Command: STORE name size <data>
// ---------------------------------------------------------------------------
void storeCommand() {
  char name[BUFSIZE];
  char sizeStr[BUFSIZE];
  waitForToken(name);
  waitForToken(sizeStr);
  int size = atoi(sizeStr);

  if (findFile(name) >= 0) {
    Serial.println(F("ERROR: file already exists"));
    return;
  }

  int start;
  if (!createFile(name, size, start)) {
    Serial.println(F("ERROR: no room for file (FAT full or disk full)"));
    return;
  }

  // Read exactly `size` data bytes from the serial port, while still letting
  // the running processes tick along.
  for (int i = 0; i < size; i++) {
    while (!Serial.available()) runProcesses();
    writeByteEEPROM(start + i, (byte)Serial.read());
  }
  // Discard anything left in the serial buffer (e.g. a trailing newline).
  while (Serial.available()) Serial.read();

  Serial.print(F("Stored file "));
  Serial.println(name);
}

// ---------------------------------------------------------------------------
// Command: RETRIEVE name
// ---------------------------------------------------------------------------
void retrieveCommand() {
  char name[BUFSIZE];
  waitForToken(name);

  int index = findFile(name);
  if (index < 0) {
    Serial.println(F("ERROR: file not found"));
    return;
  }
  FATEntry entry;
  readFATEntry(index, entry);
  for (int i = 0; i < entry.size; i++) {
    Serial.write(readByteEEPROM(entry.start + i));
  }
  Serial.println();
}

// ---------------------------------------------------------------------------
// Command: ERASE name
// ---------------------------------------------------------------------------
void eraseCommand() {
  char name[BUFSIZE];
  waitForToken(name);

  int index = findFile(name);
  if (index < 0) {
    Serial.println(F("ERROR: file not found"));
    return;
  }
  // Remove the entry by shifting every later entry one place up.
  FATEntry entry;
  for (int i = index; i < noOfFiles - 1; i++) {
    readFATEntry(i + 1, entry);
    writeFATEntry(i, entry);
  }
  noOfFiles = noOfFiles - 1;

  Serial.print(F("Erased file "));
  Serial.println(name);
}

// ---------------------------------------------------------------------------
// Command: FILES
// ---------------------------------------------------------------------------
void filesCommand() {
  Serial.print(noOfFiles);
  Serial.println(F(" file(s):"));
  FATEntry entry;
  for (int i = 0; i < noOfFiles; i++) {
    readFATEntry(i, entry);
    Serial.print(F("  "));
    Serial.print(entry.name);
    Serial.print(F("  "));
    Serial.print(entry.size);
    Serial.println(F(" bytes"));
  }
}

// ---------------------------------------------------------------------------
// Command: FREESPACE
// ---------------------------------------------------------------------------
void freespaceCommand() {
  Serial.print(F("Largest free block: "));
  Serial.print(maxFreeSpace());
  Serial.println(F(" bytes"));
}
