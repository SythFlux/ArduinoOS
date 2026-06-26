// Non-blocking serial command line. Input is read token by token so running
// processes are never held up while the user types.

#ifndef CLI_H
#define CLI_H

void cliBegin();
void handleCLI(); // call once per loop: read a token and dispatch a command

// Blocks until a whole token is typed (copied into `dest`), running processes
// meanwhile. Used by command handlers to read their arguments.
void waitForToken(char *dest);

#endif // CLI_H
