/*
 * stack.h
 * -------
 * A typed byte-stack. Each process owns a stack buffer + stack pointer (SP);
 * before doing any stack work the caller "selects" that process's stack with
 * selectStack(). All push/pop functions then operate on the selected stack.
 *
 * Value layout on the stack (bottom -> top), e.g. the INT 300:
 *      high byte (1) , low byte (44) , type tag (INT)
 * The type tag always sits on top, so a reader pops the type first and then
 * knows how many bytes the value occupies.
 *
 * Numbers are stored big-endian on the stack (most significant byte at the
 * bottom), matching the byte order produced by the `convert` tool.
 */

#ifndef STACK_H
#define STACK_H

#include <Arduino.h>

// Point all stack operations at a given process's stack buffer + SP.
void selectStack(byte *stackBuffer, byte *stackPointer);

// --- Raw bytes -------------------------------------------------------------
void pushByte(byte b);
byte popByte();
byte peekByte();   // top byte without removing it (used to read the type tag)

// --- Typed pushes (push value bytes AND the type tag) ----------------------
void pushChar(char c);
void pushInt(int i);
void pushFloat(float f);
void pushString(const char *s); // pushes chars incl. '\0', then length, then tag

// --- Typed pops (value bytes ONLY; caller has already removed the type tag) -
int popInt();
float popFloat();
char *popChars(byte length); // returns pointer to `length` bytes inside stack
char *popString();           // pops type(STRING)+length, returns the C-string

// --- Convenience -----------------------------------------------------------
// Pops a numeric value (CHAR/INT/FLOAT) including its type tag and returns it
// as a float. With peek=true the value is left on the stack.
float popVal(bool peek = false);

#endif // STACK_H
