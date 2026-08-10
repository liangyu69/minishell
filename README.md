
# minishell

A simple Unix shell written in C, supporting built-in commands, redirection, pipes, background execution, and signal handling.

## Features

### Built-in Commands

| Command | Description |
| :--- | :--- |
| `exit` | Exit the shell |
| `cd [dir]` | Change directory. Without argument, goes to HOME |
| `path` | Show current command search path |
| `path [dir...]` | Set search path (overwrites previous) |

### Redirection

| Operator | Description |
| :--- | :--- |
| `> file` | Redirect stdout to file (overwrite) |
| `< file` | Redirect stdin from file |
| Combine | `cmd < infile > outfile` |

**Example:**

```bash
grep main < myshell.c > output.txt
```

### Pipeline

| Operator | Description |
| :--- | :--- |
| `\|` | Pipe stdout of left command to stdin of right command |

**Example:**

```bash
ls -l | grep .c
```

### Background Execution

| Operator | Description |
| :--- | :--- |
| `&` | Run command in background |

**Example:**

```bash
sleep 100 &
```

### Signal Handling

| Key | Action |
| :--- | :--- |
| Ctrl+C | Terminate foreground job (shell stays alive) |
| Ctrl+D | Exit shell (EOF) |

## Build

```bash
make
```

## Usage

```bash
# Interactive mode
./minishell

# Show help
./minishell -h
```

## Examples

```bash
# Basic commands
minishell> ls -l
minishell> pwd
minishell> cd /tmp
minishell> cd

# Redirection
minishell> ls -l > out.txt
minishell> grep hello < input.txt
minishell> grep main < myshell.c > output.txt

# Pipeline
minishell> ls | grep .c
minishell> ps aux | grep bash

# Background execution
minishell> sleep 100 &
[12345] works in background

# Custom search path
minishell> path /bin /usr/bin /usr/local/bin
minishell> path
/bin /usr/bin /usr/local/bin

# Signal handling
minishell> sleep 100
# Press Ctrl+C to terminate sleep
# Shell remains alive

# Exit
minishell> exit
```

## Project Structure

```
.
├── minishell.c      # Main source file
├── Makefile         # Build configuration
├── README.md        # This file
└── .gitignore       # Git ignore rules
```

## Implementation Details

- **Command parsing:** Custom parser supporting spaces, quotes, and operators
- **Path resolution:** User-configurable search path with `path` command
- **Redirection:** `dup2()` for file descriptor duplication
- **Pipeline:** `pipe()` + `fork()` + `dup2()` for inter-process communication
- **Signal handling:** `sigaction()` for `SIGINT`, `SIGCHLD`, `SIGTSTP`
- **Background execution:** Non-blocking `fork` with `SIGCHLD` reaping

## What I Learned

- Process creation and management (`fork`, `exec`, `wait`)
- File descriptor manipulation (`open`, `dup2`, `close`)
- Inter-process communication (`pipe`)
- Signal handling (`sigaction`)
- String parsing and command-line argument processing
- Makefile basics


## Author

Liangyu

## License

MIT
