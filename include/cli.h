/*
 * cli.h
 * -----
 * The command line interface. Input is read non-blocking, token by token, so
 * running processes are never held up while the user types. A command is the
 * first token on a line; its handler reads any further argument tokens itself.
 */

#ifndef CLI_H
#define CLI_H

void cliBegin();   // print the startup prompt
void handleCLI();  // call once per loop: read a token and dispatch a command

// Blocks until a whole token has been typed, copying it into `dest`. While
// waiting it keeps the running processes going. Used by command handlers to
// read their arguments (e.g. a file name or a process id).
void waitForToken(char *dest);

#endif // CLI_H
