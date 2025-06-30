# Makefile for the FLS language interpreter (Bytecode VM version)

# Compiler
CC = clang

# Compiler flags
# -DDEBUG_TRACE_EXECUTION will print VM state before each instruction.
# -DDEBUG_PRINT_CODE will show the disassembled bytecode after compilation.
CFLAGS = -Wall -Wextra -Iinclude -g # -DDEBUG_TRACE_EXECUTION -DDEBUG_PRINT_CODE

# Source files
SRCS = src/main.c src/memory.c src/chunk.c src/debug.c src/value.c \
       src/vm.c src/compiler.c src/parser.c src/object.c src/table.c \
       src/lexer.c src/expr.c src/stmt.c src/environment.c

# Object files
OBJS = $(SRCS:.c=.o)

# Executable name
TARGET = fls.exe

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

# Run an example
run_example: all
	./$(TARGET) examples/benchmark.fls

