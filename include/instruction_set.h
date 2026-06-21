/*
 * instruction_set.h
 * -----------------
 * Numeric opcodes for the ArduinOS bytecode language (appendix C of the
 * practicum manual) plus the four data-type tags.
 *
 * IMPORTANT - clever trick:
 *   The four type tags double as the "push" instructions, AND the numeric
 *   tags CHAR/INT/FLOAT are equal to the number of bytes the value occupies
 *   (1, 2, 4). This lets the stack/memory code use the type tag directly as a
 *   byte count. STRING (3) is variable-length and always handled separately.
 *
 * IMPORTANT - compatibility:
 *   The `convert` host tool translates instruction names in a text program to
 *   these exact byte values. These values are aligned with the course-supplied
 *   instruction_set.h: opcodes 1-59 are sequential, and the flow-control /
 *   forking opcodes are 128-137 (NOT 60-69).
 */

#ifndef INSTRUCTION_SET_H
#define INSTRUCTION_SET_H

// --- Data types / push instructions (tag == byte count for numerics) -------
#define CHAR    1
#define INT     2
#define STRING  3   // variable length, handled specially
#define FLOAT   4

// --- Variables -------------------------------------------------------------
#define SET     5
#define GET     6

// --- Arithmetic ------------------------------------------------------------
#define INCREMENT 7
#define DECREMENT 8
#define PLUS      9
#define MINUS     10
#define TIMES     11
#define DIVIDEDBY 12
#define MODULUS   13
#define UNARYMINUS 14

// --- Comparison ------------------------------------------------------------
#define EQUALS              15
#define NOTEQUALS           16
#define LESSTHAN            17
#define LESSTHANOREQUALS    18
#define GREATERTHAN         19
#define GREATERTHANOREQUALS 20

// --- Logic -----------------------------------------------------------------
#define LOGICALAND 21
#define LOGICALOR  22
#define LOGICALXOR 23
#define LOGICALNOT 24

// --- Bitwise ---------------------------------------------------------------
#define BITWISEAND 25
#define BITWISEOR  26
#define BITWISEXOR 27
#define BITWISENOT 28

// --- Type conversion -------------------------------------------------------
#define TOCHAR  29
#define TOINT   30
#define TOFLOAT 31

// --- Rounding --------------------------------------------------------------
#define ROUND 32
#define FLOOR 33
#define CEIL  34

// --- Misc math -------------------------------------------------------------
#define MIN       35
#define MAX       36
#define ABS       37
#define CONSTRAIN 38
#define MAP       39
#define POW       40
#define SQ        41
#define SQRT      42

// --- Timing ----------------------------------------------------------------
#define DELAY      43
#define DELAYUNTIL 44
#define MILLIS     45

// --- I/O pins --------------------------------------------------------------
#define PINMODE      46
#define ANALOGREAD   47
#define ANALOGWRITE  48
#define DIGITALREAD  49
#define DIGITALWRITE 50

// --- Console ---------------------------------------------------------------
#define PRINT   51
#define PRINTLN 52

// --- File I/O --------------------------------------------------------------
#define OPEN       53
#define CLOSE      54
#define WRITE      55
#define READINT    56
#define READCHAR   57
#define READFLOAT  58
#define READSTRING 59

// --- Flow control ----------------------------------------------------------
// NOTE: these match the course-supplied instruction_set.h (the one `convert`
// uses). They jump from 59 to 128 - the high bit distinguishes flow-control /
// forking opcodes from data/operator opcodes.
#define IF       128
#define ELSE     129
#define ENDIF    130
#define WHILE    131
#define ENDWHILE 132
#define LOOP     133
#define ENDLOOP  134
#define STOP     135

// --- Forking ---------------------------------------------------------------
#define FORK          136
#define WAITUNTILDONE 137

#endif // INSTRUCTION_SET_H
