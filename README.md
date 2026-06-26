# DonnyOS

A tiny operating system for the Arduino Uno. It runs over the serial port: you
type commands to store files, start programs and manage them while they run.
Programs are written in a small bytecode and the OS runs several of them at once,
giving each one a turn every loop.

It all fits in the Uno's limited memory — the firmware uses about 13 KB of flash
and the file system lives in the 1 KB EEPROM so files survive a power cycle.

## How it works

- **CLI** (`cli.cpp`) — reads commands from the serial port one token at a time
  without blocking, so running programs keep ticking while you type.
- **File system** (`filesystem.cpp`) — stores files in EEPROM using a small File
  Allocation Table. Files don't have to be contiguous; free space is found from
  the gaps between them.
- **Processes** (`process.cpp`) — a table of up to 10 programs, each with its own
  state (running / paused), program counter and private stack.
- **Interpreter** (`instructions.cpp`) — runs the bytecode one instruction per
  process per loop: maths, comparisons, pin I/O, timing, file I/O, if/while
  loops and forking new processes.
- **Stack** (`stack.cpp`) — a typed byte-stack per process that the interpreter
  uses for temporary values.
- **Memory** (`memory.cpp`) — 256 bytes of working memory for program variables,
  keyed by name and owning process.

Limits and sizes are all in `include/config.h`.

## Commands

| Command | What it does |
|---|---|
| `store <name> <size>` | create a file and read `size` bytes of data into it |
| `retrieve <name>` | print a file's contents |
| `erase <name>` | delete a file |
| `files` | list all files and their sizes |
| `freespace` | show the largest free block |
| `run <name>` | start a file as a process |
| `list` | list running processes |
| `suspend <id>` | pause a process |
| `resume <id>` | resume a paused process |
| `kill <id>` | stop a process and free its variables |

## Building

Built with [PlatformIO](https://platformio.org/) for the Arduino Uno.

```
pio run            # compile
pio run -t upload  # flash to the board
```

Then open the serial monitor at 9600 baud and start typing commands.
