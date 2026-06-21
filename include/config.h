/*
 * config.h
 * --------
 * Central configuration for ArduinOS. All sizes, limits and tunable constants
 * live here so the rest of the code never hard-codes a magic number. Change a
 * value here and the whole OS adapts.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ---------------------------------------------------------------------------
// Command line interface
// ---------------------------------------------------------------------------
#define BUFSIZE 12          // Max token length (incl. terminating '\0').

// ---------------------------------------------------------------------------
// File system (stored in EEPROM)
// ---------------------------------------------------------------------------
#define MAX_FILES 10        // Number of entries in the File Allocation Table.
#define FILENAMESIZE 12     // Max file name length (incl. terminating '\0').

// ---------------------------------------------------------------------------
// Memory management
// ---------------------------------------------------------------------------
#define MEMORYSIZE 256      // Working memory in bytes (1-byte addressable).
#define MEMTABLE_SIZE 25    // Max number of variables across all processes.

// ---------------------------------------------------------------------------
// Stack (one per process)
// ---------------------------------------------------------------------------
#define STACKSIZE 32        // Bytes of stack per process.

// ---------------------------------------------------------------------------
// Processes
// ---------------------------------------------------------------------------
#define MAX_PROCESSES 10    // Max simultaneous processes in the process table.

// Process states.
#define RUNNING 'r'
#define PAUSED 'p'
#define TERMINATED 0

#endif // CONFIG_H
