// Typed per-process stack. selectStack() points the push/pop helpers at one
// process's buffer; numbers are stored big-endian (MSB at the bottom).

#include "stack.h"
#include "instruction_set.h"

static byte *gStack = nullptr; // selected stack buffer
static byte *gSP = nullptr;    // selected stack's SP byte

void selectStack(byte *stackBuffer, byte *stackPointer) {
  gStack = stackBuffer;
  gSP = stackPointer;
}

void pushByte(byte b) {
  gStack[(*gSP)++] = b;
}

byte popByte() {
  return gStack[--(*gSP)];
}

byte peekByte() {
  return gStack[*gSP - 1];
}

void pushChar(char c) {
  pushByte((byte)c);
  pushByte(CHAR);
}

void pushInt(int i) {
  pushByte(highByte(i)); // MSB first
  pushByte(lowByte(i));
  pushByte(INT);
}

void pushFloat(float f) {
  // AVR floats are little-endian; push MSB first so the stack is big-endian.
  byte *b = (byte *)&f;
  pushByte(b[3]);
  pushByte(b[2]);
  pushByte(b[1]);
  pushByte(b[0]);
  pushByte(FLOAT);
}

void pushString(const char *s) {
  byte length = 0;
  do { // include the terminating zero
    pushByte((byte)*s);
    length++;
  } while (*s++ != '\0');
  pushByte(length);
  pushByte(STRING);
}

int popInt() {
  byte low = popByte(); // low byte is on top
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

// "Pops" by lowering SP; the bytes stay in the buffer, so we return a pointer.
char *popChars(byte length) {
  *gSP -= length;
  return (char *)(gStack + *gSP);
}

char *popString() {
  popByte();               // type tag
  byte length = popByte();
  return popChars(length);
}

// Pops a numeric value of any type as a float. peek=true leaves it on the stack.
float popVal(bool peek) {
  byte savedSP = *gSP;
  byte type = popByte();
  float result = 0;
  switch (type) {
    case CHAR:  result = (float)(char)popByte(); break;
    case INT:   result = (float)popInt();        break;
    case FLOAT: result = popFloat();             break;
  }
  if (peek) *gSP = savedSP;
  return result;
}
