/*
 * stack.cpp - implementation of the typed per-process stack.
 */

#include "stack.h"
#include "instruction_set.h"

// Pointers into the *currently selected* process's stack. selectStack() makes
// these point at one process's buffer + SP so the push/pop helpers stay simple.
static byte *gStack = nullptr; // base of the selected stack buffer
static byte *gSP = nullptr;    // pointer to the selected stack's SP byte

void selectStack(byte *stackBuffer, byte *stackPointer) {
  gStack = stackBuffer;
  gSP = stackPointer;
}

// ---------------------------------------------------------------------------
// Raw bytes
// ---------------------------------------------------------------------------
void pushByte(byte b) {
  gStack[(*gSP)++] = b;
}

byte popByte() {
  return gStack[--(*gSP)];
}

byte peekByte() {
  return gStack[*gSP - 1];
}

// ---------------------------------------------------------------------------
// Typed pushes
// ---------------------------------------------------------------------------
void pushChar(char c) {
  pushByte((byte)c);
  pushByte(CHAR);
}

void pushInt(int i) {
  pushByte(highByte(i)); // MSB first -> ends up at the bottom (big-endian)
  pushByte(lowByte(i));
  pushByte(INT);
}

void pushFloat(float f) {
  // AVR stores a float little-endian in memory; we push MSB first so the stack
  // representation is big-endian (matching the `convert` tool / EEPROM).
  byte *b = (byte *)&f;
  pushByte(b[3]);
  pushByte(b[2]);
  pushByte(b[1]);
  pushByte(b[0]);
  pushByte(FLOAT);
}

void pushString(const char *s) {
  byte length = 0;
  // Push every character including the terminating zero, counting as we go.
  do {
    pushByte((byte)*s);
    length++;
  } while (*s++ != '\0');
  pushByte(length);
  pushByte(STRING);
}

// ---------------------------------------------------------------------------
// Typed pops (the type tag has already been removed by the caller)
// ---------------------------------------------------------------------------
int popInt() {
  byte low = popByte();  // low byte is on top
  byte high = popByte();
  return word(high, low);
}

float popFloat() {
  float f;
  byte *b = (byte *)&f;
  b[0] = popByte(); // LSB is on top
  b[1] = popByte();
  b[2] = popByte();
  b[3] = popByte();
  return f;
}

char *popChars(byte length) {
  *gSP -= length;                  // "pop" the bytes by lowering SP...
  return (char *)(gStack + *gSP);  // ...they remain in the buffer, so return a ptr
}

char *popString() {
  popByte();              // remove the STRING type tag
  byte length = popByte();// then the length byte
  return popChars(length);// the bytes (incl. terminating zero) stay in place
}

// ---------------------------------------------------------------------------
// Convenience: pop a numeric value of any type as a float.
// ---------------------------------------------------------------------------
float popVal(bool peek) {
  byte savedSP = *gSP;
  byte type = popByte();
  float result = 0;
  switch (type) {
    case CHAR:  result = (float)(char)popByte(); break;
    case INT:   result = (float)popInt();        break;
    case FLOAT: result = popFloat();             break;
  }
  // Popping only lowers SP; the bytes are still in the buffer. Restoring SP
  // therefore turns the pop into a peek.
  if (peek) *gSP = savedSP;
  return result;
}
