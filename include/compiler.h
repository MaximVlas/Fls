#ifndef FLS_COMPILER_H
#define FLS_COMPILER_H

#include "vm.h"

// The main entry point for the compiler. It takes source code and compiles
// it into a chunk of bytecode. Returns true on success.
bool compile(const char* source, Chunk* chunk);

#endif // FLS_COMPILER_H
