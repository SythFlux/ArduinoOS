// A typed byte-stack, one per process. Call selectStack() before push/pop.
// Each value is stored as: value bytes (big-endian), then a type tag on top.

#ifndef STACK_H
#define STACK_H

#include <Arduino.h>

void selectStack(byte *stackBuffer, byte *stackPointer);

void pushByte(byte b);
byte popByte();
byte peekByte(); // top byte without removing it

// Push value bytes AND the type tag.
void pushChar(char c);
void pushInt(int i);
void pushFloat(float f);
void pushString(const char *s);

// Pop value bytes only; caller has already removed the type tag.
int popInt();
float popFloat();
char *popChars(byte length); // pointer to `length` bytes inside the stack
char *popString();           // pops type + length, returns the C-string

// Pop a numeric value (CHAR/INT/FLOAT) as a float. peek=true leaves it on.
float popVal(bool peek = false);

#endif // STACK_H
