// All sizes, limits and tunable constants live here.

#ifndef CONFIG_H
#define CONFIG_H

#define BUFSIZE 12          // max token length (incl. '\0')

#define MAX_FILES 10        // entries in the File Allocation Table
#define FILENAMESIZE 12     // max file name length (incl. '\0')

#define MEMORYSIZE 256      // working memory in bytes
#define MEMTABLE_SIZE 25    // max variables across all processes

#define STACKSIZE 32        // stack bytes per process

#define MAX_PROCESSES 10    // max simultaneous processes

// Process states.
#define RUNNING 'r'
#define PAUSED 'p'
#define TERMINATED 0

#endif // CONFIG_H
