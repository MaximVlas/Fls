#ifndef clox_compiler_h
#define clox_compiler_h

#include "object.h"
#include "vm.h"

// Compiles source code and returns the top-level function, or NULL on error.
ObjFunction* compile(const char* source);

#endif
