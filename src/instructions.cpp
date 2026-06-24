/*
 * instructions.cpp - the ArduinOS bytecode interpreter.
 *
 * Every instruction is one byte (see instruction_set.h / appendix C). Some are
 * followed by argument bytes in the program. The interpreter uses the current
 * process's program counter (PC) to read from the EEPROM and its private stack
 * for temporary data.
 */

#include <Arduino.h>
#include "instructions.h"
#include "instruction_set.h"
#include "process.h"
#include "stack.h"
#include "memory.h"
#include "filesystem.h"

// The process currently being executed. Set at the top of execute() so the
// little helper functions below can reach its PC / FP / registers.
static ProcessType *cur = nullptr;

// Read one byte from the program at the PC and advance the PC.
static byte readPC() {
  return readByteEEPROM(cur->pc++);
}

// ---------------------------------------------------------------------------
// Read a value from EEPROM (at `addr`, advancing it) and push it on the stack.
// Shared by the CHAR/INT/FLOAT/STRING push instructions (reading from the PC)
// and by the READ* file instructions (reading from the FP).
// ---------------------------------------------------------------------------
static void readValue(int &addr, byte type) {
  if (type == STRING) {
    byte length = 0;
    byte b;
    do {
      b = readByteEEPROM(addr++);
      pushByte(b);
      length++;
    } while (b != 0); // include the terminating zero
    pushByte(length);
    pushByte(STRING);
  } else {
    // For CHAR/INT/FLOAT the type tag equals the number of bytes to read.
    for (byte i = 0; i < type; i++) pushByte(readByteEEPROM(addr++));
    pushByte(type);
  }
}

// Push a numeric result with the requested type tag.
static void pushTyped(float value, byte type) {
  switch (type) {
    case CHAR:  pushChar((char)(long)value); break;
    case INT:   pushInt((int)(long)value);   break;
    case FLOAT: pushFloat(value);            break;
  }
}

// ---------------------------------------------------------------------------
// Operators
// ---------------------------------------------------------------------------

// Unary: pop one value, compute, push the result (type per appendix table 2).
static void unaryOp(byte op) {
  byte type = peekByte();
  float x = popVal();
  byte resultType = type; // default: same type as the operand
  float r = 0;
  switch (op) {
    case INCREMENT:  r = x + 1;            break;
    case DECREMENT:  r = x - 1;            break;
    case UNARYMINUS: r = -x;               break;
    case ABS:        r = fabs(x);          break;
    case SQ:         r = x * x;            break;
    case SQRT:       r = sqrt(x);          break;
    case ANALOGREAD: r = analogRead((int)x); resultType = INT;  break;
    case DIGITALREAD:r = digitalRead((int)x);resultType = CHAR; break;
    case LOGICALNOT: r = (x == 0) ? 1 : 0;   resultType = CHAR; break;
    case BITWISENOT: r = (float)(~(long)x);  /* keeps operand type */ break;
    case TOCHAR:     r = x; resultType = CHAR;  break;
    case TOINT:      r = x; resultType = INT;   break;
    case TOFLOAT:    r = x; resultType = FLOAT; break;
    case ROUND:      r = round(x); resultType = INT; break;
    case FLOOR:      r = floor(x); resultType = INT; break;
    case CEIL:       r = ceil(x);  resultType = INT; break;
  }
  pushTyped(r, resultType);
}

// Binary: pop two values, compute, push the result. Note y is on top of x, so
// for "x - y" we pop y first, then x.
static void binaryOp(byte op) {
  byte typeY = peekByte(); float y = popVal();
  byte typeX = peekByte(); float x = popVal();
  byte resultType = (typeX > typeY) ? typeX : typeY; // most precise of the two
  float r = 0;
  switch (op) {
    case PLUS:      r = x + y; break;
    case MINUS:     r = x - y; break;
    case TIMES:     r = x * y; break;
    case DIVIDEDBY: r = x / y; break;
    case MODULUS:   r = (float)((long)x % (long)y); break;
    case MIN:       r = (x < y) ? x : y; break;
    case MAX:       r = (x > y) ? x : y; break;
    case POW:       r = pow(x, y); break;
    case BITWISEAND:r = (float)((long)x & (long)y); break;
    case BITWISEOR: r = (float)((long)x | (long)y); break;
    case BITWISEXOR:r = (float)((long)x ^ (long)y); break;
    // Comparisons and logic always yield a CHAR (0 or 1).
    case EQUALS:              r = (x == y); resultType = CHAR; break;
    case NOTEQUALS:           r = (x != y); resultType = CHAR; break;
    case LESSTHAN:            r = (x <  y); resultType = CHAR; break;
    case LESSTHANOREQUALS:    r = (x <= y); resultType = CHAR; break;
    case GREATERTHAN:         r = (x >  y); resultType = CHAR; break;
    case GREATERTHANOREQUALS: r = (x >= y); resultType = CHAR; break;
    case LOGICALAND: r = (x != 0 && y != 0);        resultType = CHAR; break;
    case LOGICALOR:  r = (x != 0 || y != 0);        resultType = CHAR; break;
    case LOGICALXOR: r = ((x != 0) ^ (y != 0));     resultType = CHAR; break;
  }
  pushTyped(r, resultType);
}

// ---------------------------------------------------------------------------
// Console output (PRINT / PRINTLN)
// ---------------------------------------------------------------------------
static void printValue(bool newline) {
  byte type = popByte();
  switch (type) {
    case CHAR:  Serial.print((char)popByte()); break;
    case INT:   Serial.print(popInt());        break;
    case FLOAT: Serial.print(popFloat());      break;
    case STRING: {
      byte length = popByte();
      Serial.print(popChars(length)); // null-terminated inside the stack buffer
      break;
    }
  }
  if (newline) Serial.print('\n');
}

// ---------------------------------------------------------------------------
// File I/O
// ---------------------------------------------------------------------------
static void openFile() {
  long length = (long)popVal(); // size argument (on top)
  char *name = popString();     // file name beneath it
  int start, size;
  if (!getFileInfo(name, start, size)) {
    // File does not exist yet: create it with the requested length.
    if (!createFile(name, (int)length, start)) {
      Serial.println(F("ERROR: cannot open/create file"));
      return;
    }
  }
  cur->fp = start; // file pointer to the beginning of the file
}

static void writeValue() {
  byte type = popByte();
  if (type == STRING) {
    byte length = popByte();
    char *s = popChars(length);
    for (byte i = 0; i < length; i++) writeByteEEPROM(cur->fp++, (byte)s[i]);
  } else {
    // Pop value bytes (top-first) into a buffer, then write them MSB-first.
    byte buf[4];
    for (int i = type - 1; i >= 0; i--) buf[i] = popByte();
    for (byte i = 0; i < type; i++) writeByteEEPROM(cur->fp++, buf[i]);
  }
}

// ---------------------------------------------------------------------------
// The interpreter
// ---------------------------------------------------------------------------
void execute(int index) {
  cur = &processTable[index];
  selectStack(cur->stack, &cur->sp);

  byte op = readByteEEPROM(cur->pc++);
  switch (op) {
    // --- push literals ---
    case CHAR:
    case INT:
    case FLOAT:
    case STRING:
      readValue(cur->pc, op);
      break;

    // --- variables ---
    case SET: storeVariable(readPC(), cur->pid); break;
    case GET: readVariable(readPC(), cur->pid);  break;

    // --- unary operators ---
    case INCREMENT: case DECREMENT: case UNARYMINUS: case ABS: case SQ:
    case SQRT: case ANALOGREAD: case DIGITALREAD: case LOGICALNOT:
    case BITWISENOT: case TOCHAR: case TOINT: case TOFLOAT:
    case ROUND: case FLOOR: case CEIL:
      unaryOp(op);
      break;

    // --- binary operators ---
    case PLUS: case MINUS: case TIMES: case DIVIDEDBY: case MODULUS:
    case MIN: case MAX: case POW:
    case BITWISEAND: case BITWISEOR: case BITWISEXOR:
    case EQUALS: case NOTEQUALS: case LESSTHAN: case LESSTHANOREQUALS:
    case GREATERTHAN: case GREATERTHANOREQUALS:
    case LOGICALAND: case LOGICALOR: case LOGICALXOR:
      binaryOp(op);
      break;

    // --- ternary / 5-ary math ---
    case CONSTRAIN: {
      byte tz = peekByte(); float z = popVal();
      byte ty = peekByte(); float y = popVal();
      byte tx = peekByte(); float x = popVal();
      byte rt = max(tx, max(ty, tz));
      pushTyped(min(max(x, y), z), rt);
      break;
    }
    case MAP: {
      byte t5 = peekByte(); float e = popVal();
      byte t4 = peekByte(); float d = popVal();
      byte t3 = peekByte(); float c = popVal();
      byte t2 = peekByte(); float b = popVal();
      byte t1 = peekByte(); float a = popVal();
      byte rt = max(t1, max(t2, max(t3, max(t4, t5))));
      pushTyped((a - b) * (e - d) / (c - b) + d, rt);
      break;
    }

    // --- console ---
    case PRINT:   printValue(false); break;
    case PRINTLN: printValue(true);  break;

    // --- I/O pins ---
    case PINMODE:      { int m = (int)popVal(); int p = (int)popVal(); pinMode(p, m); break; }
    case ANALOGWRITE:  { int v = (int)popVal(); int p = (int)popVal(); analogWrite(p, v); break; }
    case DIGITALWRITE: { int v = (int)popVal(); int p = (int)popVal(); digitalWrite(p, v); break; }

    // --- timing ---
    case DELAY:  delay((unsigned long)popVal()); break;
    case MILLIS: pushInt((int)millis());         break;
    case DELAYUNTIL: {
      float target = popVal(true); // peek
      // The millisecond counter is 16-bit (see MILLIS) and wraps every ~65 s,
      // so a plain `target > now` comparison breaks at the wrap boundary and
      // the loop runs full speed. Compare with a signed 16-bit difference,
      // which stays correct across the wrap (valid for delays up to ~32 s).
      unsigned int now16 = (unsigned int)millis();
      unsigned int tgt16 = (unsigned int)(long)target;
      if ((int)(tgt16 - now16) > 0) cur->pc -= 1; // target still ahead: wait
      else popVal();                              // reached: consume the value
      break;
    }

    // --- file I/O ---
    case OPEN:       openFile(); break;
    case CLOSE:      /* nothing to do */ break;
    case WRITE:      writeValue(); break;
    case READCHAR:   readValue(cur->fp, CHAR);   break;
    case READINT:    readValue(cur->fp, INT);    break;
    case READFLOAT:  readValue(cur->fp, FLOAT);  break;
    case READSTRING: readValue(cur->fp, STRING); break;

    // --- flow control ---
    case IF: {
      byte len = readPC();
      if (popVal(true) == 0) cur->pc += len; // jump over the true-body if false
      break;
    }
    case ELSE: {
      byte len = readPC();
      if (popVal(true) != 0) cur->pc += len; // jump over the else-body if true
      break;
    }
    case ENDIF: popVal(); break; // discard the condition value

    case WHILE: {
      byte condLen = readPC();
      byte bodyLen = readPC();
      if (popVal() == 0) {
        cur->pc += bodyLen + 1; // skip the body (and the ENDWHILE byte)
      } else {
        pushByte(condLen + bodyLen + 4); // ENDWHILE jump-back distance
      }
      break;
    }
    case ENDWHILE: { byte back = popByte(); cur->pc -= back; break; }

    case LOOP:    cur->loopReg = cur->pc; break;     // remember where to return
    case ENDLOOP: cur->pc = cur->loopReg; break;     // jump back to after LOOP

    case STOP:
      clearProcessVariables(cur->pid);
      cur->state = TERMINATED;
      break;

    // --- forking ---
    case FORK: {
      char *name = popString();
      int pid = startProcess(name);
      if (pid < 0) Serial.println(F("ERROR: fork failed"));
      pushInt(pid);
      break;
    }
    case WAITUNTILDONE: {
      int pid = (int)popVal(true); // peek
      if (findProcessByPid(pid) >= 0) cur->pc -= 1; // still alive: keep waiting
      else popVal();                                // done: consume the value
      break;
    }

    default:
      // Unknown opcode: terminate the process to avoid a runaway.
      Serial.print(F("ERROR: unknown opcode "));
      Serial.println(op);
      cur->state = TERMINATED;
      break;
  }
}

// ---------------------------------------------------------------------------
// Advance every running process by one instruction (declared in process.h).
// ---------------------------------------------------------------------------
void runProcesses() {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (processTable[i].state == RUNNING) execute(i);
  }
}
