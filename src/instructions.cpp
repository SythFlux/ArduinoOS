// The bytecode interpreter. Every instruction is one byte (see
// instruction_set.h); some are followed by argument bytes in the program.

#include <Arduino.h>
#include "instructions.h"
#include "instruction_set.h"
#include "process.h"
#include "stack.h"
#include "memory.h"
#include "filesystem.h"

// The process currently running, set at the top of execute().
static ProcessType *cur = nullptr;

static byte readPC() {
  return readByteEEPROM(cur->pc++);
}

// Reads a value from EEPROM at `addr` (advancing it) and pushes it. Used both
// for the literal push instructions (from the PC) and the READ* file ones
// (from the FP).
static void readValue(int &addr, byte type) {
  if (type == STRING) {
    byte length = 0;
    byte b;
    do {
      b = readByteEEPROM(addr++);
      pushByte(b);
      length++;
    } while (b != 0);
    pushByte(length);
    pushByte(STRING);
  } else {
    for (byte i = 0; i < type; i++) pushByte(readByteEEPROM(addr++));
    pushByte(type);
  }
}

static void pushTyped(float value, byte type) {
  switch (type) {
    case CHAR:  pushChar((char)(long)value); break;
    case INT:   pushInt((int)(long)value);   break;
    case FLOAT: pushFloat(value);            break;
  }
}

static void unaryOp(byte op) {
  byte type = peekByte();
  float x = popVal();
  byte resultType = type;
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
    case BITWISENOT: r = (float)(~(long)x);  break;
    case TOCHAR:     r = x; resultType = CHAR;  break;
    case TOINT:      r = x; resultType = INT;   break;
    case TOFLOAT:    r = x; resultType = FLOAT; break;
    case ROUND:      r = round(x); resultType = INT; break;
    case FLOOR:      r = floor(x); resultType = INT; break;
    case CEIL:       r = ceil(x);  resultType = INT; break;
  }
  pushTyped(r, resultType);
}

// y is on top of x, so for "x - y" we pop y first.
static void binaryOp(byte op) {
  byte typeY = peekByte(); float y = popVal();
  byte typeX = peekByte(); float x = popVal();
  byte resultType = (typeX > typeY) ? typeX : typeY;
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

static void printValue(bool newline) {
  byte type = popByte();
  switch (type) {
    case CHAR:  Serial.print((char)popByte()); break;
    case INT:   Serial.print(popInt());        break;
    case FLOAT: Serial.print(popFloat());      break;
    case STRING: {
      byte length = popByte();
      Serial.print(popChars(length));
      break;
    }
  }
  if (newline) Serial.print('\n');
}

static void openFile() {
  long length = (long)popVal();
  char *name = popString();
  int start, size;
  if (!getFileInfo(name, start, size)) {
    // Doesn't exist yet: create it at the requested length.
    if (!createFile(name, (int)length, start)) {
      Serial.println(F("ERROR: cannot open/create file"));
      return;
    }
  }
  cur->fp = start;
}

static void writeValue() {
  byte type = popByte();
  if (type == STRING) {
    byte length = popByte();
    char *s = popChars(length);
    for (byte i = 0; i < length; i++) writeByteEEPROM(cur->fp++, (byte)s[i]);
  } else {
    // Pop value bytes top-first into a buffer, then write them MSB-first.
    byte buf[4];
    for (int i = type - 1; i >= 0; i--) buf[i] = popByte();
    for (byte i = 0; i < type; i++) writeByteEEPROM(cur->fp++, buf[i]);
  }
}

void execute(int index) {
  cur = &processTable[index];
  selectStack(cur->stack, &cur->sp);

  byte op = readByteEEPROM(cur->pc++);
  switch (op) {
    case CHAR:
    case INT:
    case FLOAT:
    case STRING:
      readValue(cur->pc, op);
      break;

    case SET: storeVariable(readPC(), cur->pid); break;
    case GET: readVariable(readPC(), cur->pid);  break;

    case INCREMENT: case DECREMENT: case UNARYMINUS: case ABS: case SQ:
    case SQRT: case ANALOGREAD: case DIGITALREAD: case LOGICALNOT:
    case BITWISENOT: case TOCHAR: case TOINT: case TOFLOAT:
    case ROUND: case FLOOR: case CEIL:
      unaryOp(op);
      break;

    case PLUS: case MINUS: case TIMES: case DIVIDEDBY: case MODULUS:
    case MIN: case MAX: case POW:
    case BITWISEAND: case BITWISEOR: case BITWISEXOR:
    case EQUALS: case NOTEQUALS: case LESSTHAN: case LESSTHANOREQUALS:
    case GREATERTHAN: case GREATERTHANOREQUALS:
    case LOGICALAND: case LOGICALOR: case LOGICALXOR:
      binaryOp(op);
      break;

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

    case PRINT:   printValue(false); break;
    case PRINTLN: printValue(true);  break;

    case PINMODE:      { int m = (int)popVal(); int p = (int)popVal(); pinMode(p, m); break; }
    case ANALOGWRITE:  { int v = (int)popVal(); int p = (int)popVal(); analogWrite(p, v); break; }
    case DIGITALWRITE: { int v = (int)popVal(); int p = (int)popVal(); digitalWrite(p, v); break; }

    case DELAY:  delay((unsigned long)popVal()); break;
    case MILLIS: pushInt((int)millis());         break;
    case DELAYUNTIL: {
      float target = popVal(true); // peek
      // MILLIS is 16-bit and wraps every ~65 s, so compare with a signed 16-bit
      // difference to stay correct across the wrap (valid for delays up to ~32 s).
      unsigned int now16 = (unsigned int)millis();
      unsigned int tgt16 = (unsigned int)(long)target;
      if ((int)(tgt16 - now16) > 0) cur->pc -= 1; // not there yet: wait
      else popVal();
      break;
    }

    case OPEN:       openFile(); break;
    case CLOSE:      break;
    case WRITE:      writeValue(); break;
    case READCHAR:   readValue(cur->fp, CHAR);   break;
    case READINT:    readValue(cur->fp, INT);    break;
    case READFLOAT:  readValue(cur->fp, FLOAT);  break;
    case READSTRING: readValue(cur->fp, STRING); break;

    case IF: {
      byte len = readPC();
      if (popVal(true) == 0) cur->pc += len; // skip the true-body if false
      break;
    }
    case ELSE: {
      byte len = readPC();
      if (popVal(true) != 0) cur->pc += len; // skip the else-body if true
      break;
    }
    case ENDIF: popVal(); break;

    case WHILE: {
      byte condLen = readPC();
      byte bodyLen = readPC();
      if (popVal() == 0) {
        cur->pc += bodyLen + 1; // skip the body and the ENDWHILE byte
      } else {
        pushByte(condLen + bodyLen + 4); // ENDWHILE jump-back distance
      }
      break;
    }
    case ENDWHILE: { byte back = popByte(); cur->pc -= back; break; }

    case LOOP:    cur->loopReg = cur->pc; break;
    case ENDLOOP: cur->pc = cur->loopReg; break;

    case STOP:
      clearProcessVariables(cur->pid);
      cur->state = TERMINATED;
      break;

    case FORK: {
      char *name = popString();
      int pid = startProcess(name);
      if (pid < 0) Serial.println(F("ERROR: fork failed"));
      pushInt(pid);
      break;
    }
    case WAITUNTILDONE: {
      int pid = (int)popVal(true); // peek
      if (findProcessByPid(pid) >= 0) cur->pc -= 1; // still alive: wait
      else popVal();
      break;
    }

    default:
      Serial.print(F("ERROR: unknown opcode "));
      Serial.println(op);
      cur->state = TERMINATED;
      break;
  }
}

void runProcesses() {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (processTable[i].state == RUNNING) execute(i);
  }
}
