#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "interpreter.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "environment.h"

static Interpreter interpreter;

// Forward declarations
static Value evaluate(Expr* expr);
static InterpretResult execute(Stmt* stmt);
static InterpretResult executeBlock(Stmt** statements, Environment* environment);

// --- Error and Native Function Handling ---

void runtimeError(Token token, const char* format, ...) {
    if (interpreter.hadRuntimeError) return;
    interpreter.hadRuntimeError = 1;

    fprintf(stderr, "[line %d] Error", token.line);
    if (token.type != TOKEN_ERROR) {
        fprintf(stderr, " at '%.*s'", token.length, token.start);
    }
    
    fprintf(stderr, ": ");

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);
}

static Value clockNative(int argCount, Value* args) {
    (void)args; // Unused parameter
    if (argCount != 0) {
        // We don't have a token, so we can't report a line number easily.
        fprintf(stderr, "clock() expects 0 arguments but got %d.\n", argCount);
        interpreter.hadRuntimeError = true;
        return NIL_VAL;
    }
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

// --- Core Execution Logic ---

static Value call(Value callee, int argCount, Value* args) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_FUNCTION: {
                ObjFunction* function = AS_FUNCTION(callee);
                if (argCount != function->arity) {
                    fprintf(stderr, "Expected %d arguments but got %d.\n", function->arity, argCount);
                    interpreter.hadRuntimeError = true;
                    return NIL_VAL;
                }
                
                Environment frame;
                initEnvironment(&frame, interpreter.environment);
                
                for (int i = 0; i < function->arity; i++) {
                    ObjString* paramName = copyString(function->params[i].start, function->params[i].length);
                    defineVariable(&frame, paramName, args[i]);
                }

                Stmt* bodyBlock = function->body;
                InterpretResult result = executeBlock(bodyBlock->as.block.statements, &frame);
                
                freeEnvironment(&frame);

                if (result == INTERPRET_RETURN) {
                    return interpreter.returnValue;
                }
                return NIL_VAL;
            }
            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                return native(argCount, args);
            }
            default:
                break; // Fall through to error.
        }
    }
    fprintf(stderr, "Can only call functions and classes.\n");
    interpreter.hadRuntimeError = true;
    return NIL_VAL;
}

static Value evaluate(Expr* expr) {
    if (interpreter.hadRuntimeError) return NIL_VAL;

    switch (expr->type) {
        case EXPR_LITERAL: return expr->as.literal.value;
        case EXPR_GROUPING: return evaluate(expr->as.grouping.expression);
        case EXPR_VARIABLE: {
            Value value;
            if (!getVariable(interpreter.environment, &expr->as.variable.name, &value)) {
                interpreter.hadRuntimeError = true;
                return NIL_VAL;
            }
            return value;
        }
        case EXPR_ASSIGN: {
            Value value = evaluate(expr->as.assign.value);
            if (interpreter.hadRuntimeError) return NIL_VAL;
            if (!assignVariable(interpreter.environment, &expr->as.assign.name, value)) {
                interpreter.hadRuntimeError = true;
                return NIL_VAL;
            }
            return value;
        }
        case EXPR_CALL: {
            Value callee = evaluate(expr->as.call.callee);
            if (interpreter.hadRuntimeError) return NIL_VAL;
            Value callArgs[256];
            for (int i = 0; i < expr->as.call.argCount; i++) {
                callArgs[i] = evaluate(expr->as.call.arguments[i]);
                if (interpreter.hadRuntimeError) return NIL_VAL;
            }
            return call(callee, expr->as.call.argCount, callArgs);
        }
        case EXPR_LOGICAL: {
            Value left = evaluate(expr->as.logical.left);
            if (interpreter.hadRuntimeError) return NIL_VAL;
            if (expr->as.logical.operator.type == TOKEN_OR) {
                if (!isFalsey(left)) return left;
            } else {
                if (isFalsey(left)) return left;
            }
            return evaluate(expr->as.logical.right);
        }
        case EXPR_UNARY: {
            Value right = evaluate(expr->as.unary.right);
            if (interpreter.hadRuntimeError) return NIL_VAL;
            switch (expr->as.unary.operator.type) {
                case TOKEN_BANG: return BOOL_VAL(isFalsey(right));
                case TOKEN_MINUS:
                    if (!IS_NUMBER(right)) { runtimeError(expr->as.unary.operator, "Operand must be a number."); return NIL_VAL; }
                    return NUMBER_VAL(-AS_NUMBER(right));
                default: break;
            }
        }
        case EXPR_BINARY: {
            Value left = evaluate(expr->as.binary.left);
            if (interpreter.hadRuntimeError) return NIL_VAL;
            Value right = evaluate(expr->as.binary.right);
            if (interpreter.hadRuntimeError) return NIL_VAL;
            Token op = expr->as.binary.operator;
            switch (op.type) {
                case TOKEN_GREATER: if (!IS_NUMBER(left) || !IS_NUMBER(right)) { runtimeError(op, "Operands must be numbers."); return NIL_VAL; } return BOOL_VAL(AS_NUMBER(left) > AS_NUMBER(right));
                case TOKEN_GREATER_EQUAL: if (!IS_NUMBER(left) || !IS_NUMBER(right)) { runtimeError(op, "Operands must be numbers."); return NIL_VAL; } return BOOL_VAL(AS_NUMBER(left) >= AS_NUMBER(right));
                case TOKEN_LESS: if (!IS_NUMBER(left) || !IS_NUMBER(right)) { runtimeError(op, "Operands must be numbers."); return NIL_VAL; } return BOOL_VAL(AS_NUMBER(left) < AS_NUMBER(right));
                case TOKEN_LESS_EQUAL: if (!IS_NUMBER(left) || !IS_NUMBER(right)) { runtimeError(op, "Operands must be numbers."); return NIL_VAL; } return BOOL_VAL(AS_NUMBER(left) <= AS_NUMBER(right));
                case TOKEN_BANG_EQUAL: return BOOL_VAL(AS_BOOL(left) != AS_BOOL(right));
                case TOKEN_EQUAL_EQUAL: return BOOL_VAL(AS_BOOL(left) == AS_BOOL(right));
                case TOKEN_MINUS: if (!IS_NUMBER(left) || !IS_NUMBER(right)) { runtimeError(op, "Operands must be numbers."); return NIL_VAL; } return NUMBER_VAL(AS_NUMBER(left) - AS_NUMBER(right));
                case TOKEN_PLUS:
                    if (IS_NUMBER(left) && IS_NUMBER(right)) { return NUMBER_VAL(AS_NUMBER(left) + AS_NUMBER(right)); }
                    if (IS_STRING(left) && IS_STRING(right)) {
                        int length = AS_STRING(left)->length + AS_STRING(right)->length;
                        char* chars = ALLOCATE(char, length + 1);
                        memcpy(chars, AS_CSTRING(left), AS_STRING(left)->length);
                        memcpy(chars + AS_STRING(left)->length, AS_CSTRING(right), AS_STRING(right)->length);
                        chars[length] = '\0';
                        return OBJ_VAL(takeString(chars, length));
                    }
                    runtimeError(op, "Operands must be two numbers or two strings.");
                    return NIL_VAL;
                case TOKEN_SLASH: if (!IS_NUMBER(left) || !IS_NUMBER(right)) { runtimeError(op, "Operands must be numbers."); return NIL_VAL; } if (AS_NUMBER(right) == 0) { runtimeError(op, "Division by zero."); return NIL_VAL; } return NUMBER_VAL(AS_NUMBER(left) / AS_NUMBER(right));
                case TOKEN_STAR: if (!IS_NUMBER(left) || !IS_NUMBER(right)) { runtimeError(op, "Operands must be numbers."); return NIL_VAL; } return NUMBER_VAL(AS_NUMBER(left) * AS_NUMBER(right));
                default: break;
            }
        }
        default: break;
    }
    return NIL_VAL;
}

static InterpretResult execute(Stmt* stmt) {
    if (interpreter.hadRuntimeError) return INTERPRET_RUNTIME_ERROR;

    switch (stmt->type) {
        case STMT_BLOCK: {
            Environment blockEnv;
            initEnvironment(&blockEnv, interpreter.environment);
            InterpretResult result = executeBlock(stmt->as.block.statements, &blockEnv);
            freeEnvironment(&blockEnv);
            return result;
        }
        case STMT_EXPRESSION:
            evaluate(stmt->as.expression.expression);
            return INTERPRET_OK;
        case STMT_FUNCTION: {
            ObjFunction* function = newFunction(stmt->as.function.name, stmt->as.function.params, stmt->as.function.arity, stmt->as.function.body);
            defineVariable(interpreter.environment, function->name, OBJ_VAL(function));
            return INTERPRET_OK;
        }
        case STMT_IF: {
            Value condition = evaluate(stmt->as.ifStmt.condition);
            if (interpreter.hadRuntimeError) return INTERPRET_RUNTIME_ERROR;
            if (!isFalsey(condition)) {
                return execute(stmt->as.ifStmt.thenBranch);
            } else if (stmt->as.ifStmt.elseBranch != NULL) {
                return execute(stmt->as.ifStmt.elseBranch);
            }
            return INTERPRET_OK;
        }
        case STMT_PRINT: {
            Value value = evaluate(stmt->as.print.expression);
            if (interpreter.hadRuntimeError) return INTERPRET_RUNTIME_ERROR;
            printValue(value);
            printf("\n");
            return INTERPRET_OK;
        }
        case STMT_RETURN: {
            if (stmt->as.returnStmt.value != NULL) {
                interpreter.returnValue = evaluate(stmt->as.returnStmt.value);
                if (interpreter.hadRuntimeError) return INTERPRET_RUNTIME_ERROR;
            } else {
                interpreter.returnValue = NIL_VAL;
            }
            return INTERPRET_RETURN;
        }
        case STMT_VAR: {
            Value value = NIL_VAL;
            if (stmt->as.var.initializer != NULL) {
                value = evaluate(stmt->as.var.initializer);
                if (interpreter.hadRuntimeError) return INTERPRET_RUNTIME_ERROR;
            }
            ObjString* name = copyString(stmt->as.var.name.start, stmt->as.var.name.length);
            defineVariable(interpreter.environment, name, value);
            return INTERPRET_OK;
        }
        case STMT_WHILE: {
            while (!isFalsey(evaluate(stmt->as.whileStmt.condition)) && !interpreter.hadRuntimeError) {
                InterpretResult result = execute(stmt->as.whileStmt.body);
                if (result != INTERPRET_OK) return result;
            }
            return INTERPRET_OK;
        }
    }
    return INTERPRET_OK;
}

static InterpretResult executeBlock(Stmt** statements, Environment* environment) {
    Environment* previous = interpreter.environment;
    interpreter.environment = environment;
    InterpretResult result = INTERPRET_OK;

    for (int i = 0; statements[i] != NULL; i++) {
        result = execute(statements[i]);
        if (result != INTERPRET_OK) {
            break;
        }
    }

    interpreter.environment = previous;
    return result;
}

void defineNative(const char* name, NativeFn function, int arity) {
    ObjString* objName = copyString(name, (int)strlen(name));
    ObjNative* native = newNative(function, arity);
    defineVariable(interpreter.environment, objName, OBJ_VAL(native));
}

void initInterpreter() {
    interpreter.environment = (Environment*)malloc(sizeof(Environment));
    initEnvironment(interpreter.environment, NULL);
    initStrings();
    interpreter.hadRuntimeError = false;
    defineNative("clock", clockNative, 0);
}

void freeInterpreter() {
    freeEnvironment(interpreter.environment);
    free(interpreter.environment);
    freeStrings();
}

InterpretResult interpret(Stmt** statements) {
    if (statements == NULL) return INTERPRET_COMPILE_ERROR;
    
    for (int i = 0; statements[i] != NULL; i++) {
        execute(statements[i]);
        if(interpreter.hadRuntimeError) return INTERPRET_RUNTIME_ERROR;
    }

    return INTERPRET_OK;
}
