/*
 * memory.cpp - variable storage in working memory.
 */

#include "memory.h"
#include "stack.h"
#include "instruction_set.h"

static byte memory[MEMORYSIZE];        // the 256-byte working memory
static MemEntry memTable[MEMTABLE_SIZE]; // one entry per stored variable
static byte noOfVars = 0;              // number of variables currently in use

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Returns the memory-table index of variable (name, processId), or -1.
static int findVariable(byte name, int processId) {
  for (byte i = 0; i < noOfVars; i++) {
    if (memTable[i].name == name && memTable[i].processId == processId) return i;
  }
  return -1;
}

// Removes the table entry at `index`, shifting later entries up.
static void removeEntry(int index) {
  for (byte i = index; i < noOfVars - 1; i++) memTable[i] = memTable[i + 1];
  noOfVars--;
}

// Finds a free block of `size` bytes in memory using the same gap-search as
// the file system: sort entries by address and look between them. Returns the
// start address, or -1 if there is no room.
static int findMemorySlot(int size) {
  // Indices of table entries sorted by address (insertion sort, small N).
  byte order[MEMTABLE_SIZE];
  for (byte i = 0; i < noOfVars; i++) order[i] = i;
  for (byte i = 1; i < noOfVars; i++) {
    byte key = order[i];
    int j = i - 1;
    while (j >= 0 && memTable[order[j]].address > memTable[key].address) {
      order[j + 1] = order[j];
      j--;
    }
    order[j + 1] = key;
  }

  int prevEnd = 0;
  for (byte i = 0; i < noOfVars; i++) {
    MemEntry &e = memTable[order[i]];
    if (e.address - prevEnd >= size) return prevEnd;
    prevEnd = e.address + e.size;
  }
  if (MEMORYSIZE - prevEnd >= size) return prevEnd;
  return -1;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void storeVariable(byte name, int processId) {
  if (noOfVars >= MEMTABLE_SIZE) {
    Serial.println(F("ERROR: memory table full"));
    return;
  }

  // Overwrite: drop any existing variable with the same name + process.
  int existing = findVariable(name, processId);
  if (existing >= 0) removeEntry(existing);

  // Pop the type tag; for numerics the tag equals the byte count, for a string
  // the next byte on the stack holds the length.
  byte type = popByte();
  int size = (type == STRING) ? popByte() : type;

  int address = findMemorySlot(size);
  if (address < 0) {
    Serial.println(F("ERROR: out of memory"));
    return;
  }

  // The value's bytes are on the stack with the MSB at the bottom; popping
  // yields them top-first, so we write them back-to-front to keep big-endian
  // order in memory.
  for (int i = size - 1; i >= 0; i--) memory[address + i] = popByte();

  memTable[noOfVars] = { name, type, (byte)address, (byte)size, processId };
  noOfVars++;
}

void readVariable(byte name, int processId) {
  int index = findVariable(name, processId);
  if (index < 0) {
    Serial.println(F("ERROR: variable not found"));
    return;
  }
  MemEntry &e = memTable[index];

  for (byte i = 0; i < e.size; i++) pushByte(memory[e.address + i]);
  if (e.type == STRING) pushByte(e.size); // strings also carry their length
  pushByte(e.type);
}

void clearProcessVariables(int processId) {
  // Walk the table; whenever we delete an entry, keep the index so we re-check
  // the entry that shifted into this slot.
  byte i = 0;
  while (i < noOfVars) {
    if (memTable[i].processId == processId) {
      removeEntry(i);
    } else {
      i++;
    }
  }
}
