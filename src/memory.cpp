// Variable storage in working memory.

#include "memory.h"
#include "stack.h"
#include "instruction_set.h"

static byte memory[MEMORYSIZE];
static MemEntry memTable[MEMTABLE_SIZE];
static byte noOfVars = 0;

static int findVariable(byte name, int processId) {
  for (byte i = 0; i < noOfVars; i++) {
    if (memTable[i].name == name && memTable[i].processId == processId) return i;
  }
  return -1;
}

static void removeEntry(int index) {
  for (byte i = index; i < noOfVars - 1; i++) memTable[i] = memTable[i + 1];
  noOfVars--;
}

// Finds a free block of `size` bytes: sort entries by address, look in the gaps.
static int findMemorySlot(int size) {
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

void storeVariable(byte name, int processId) {
  if (noOfVars >= MEMTABLE_SIZE) {
    Serial.println(F("ERROR: memory table full"));
    return;
  }

  int existing = findVariable(name, processId);
  if (existing >= 0) removeEntry(existing); // overwrite same name + process

  // For numerics the type tag is the byte count; for a string the next byte
  // on the stack holds the length.
  byte type = popByte();
  int size = (type == STRING) ? popByte() : type;

  int address = findMemorySlot(size);
  if (address < 0) {
    Serial.println(F("ERROR: out of memory"));
    return;
  }

  // Stack holds the value MSB-first, so write back-to-front to keep big-endian.
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
  if (e.type == STRING) pushByte(e.size);
  pushByte(e.type);
}

void clearProcessVariables(int processId) {
  byte i = 0;
  while (i < noOfVars) {
    if (memTable[i].processId == processId) {
      removeEntry(i); // don't advance i; the next entry shifted down
    } else {
      i++;
    }
  }
}

byte countProcessVariables(int processId) {
  byte count = 0;
  for (byte i = 0; i < noOfVars; i++) {
    if (memTable[i].processId == processId) count++;
  }
  return count;
}
