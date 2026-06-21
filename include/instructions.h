/*
 * instructions.h
 * --------------
 * The bytecode interpreter. execute() reads and runs one instruction for the
 * process at the given process-table index. runProcesses() (declared in
 * process.h) calls execute() once per RUNNING process each loop iteration.
 */

#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

// Read and execute a single instruction for processTable[index]. Advances that
// process's program counter and operates on its private stack.
void execute(int index);

#endif // INSTRUCTIONS_H
