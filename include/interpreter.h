#ifndef FLS_INTERPRETER_H
#define FLS_INTERPRETER_H

#include "stmt.h"
#include "environment.h"
#include "value.h"

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
    INTERPRET_RETURN,
} InterpretResult;

typedef struct {
    Environment* environment;
    int hadRuntimeError;
    Value returnValue;
} Interpreter;

void initInterpreter();
void freeInterpreter();
InterpretResult interpret(Stmt** statements);
void runtimeError(Token token, const char* format, ...);

#endif // FLS_INTERPRETER_H
