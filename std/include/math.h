#ifndef FLS_STD_MATH_H
#define FLS_STD_MATH_H

#include "value.h"

void initMathLibrary();
Value sqrtNative(int argCount, Value* args);
Value sinNative(int argCount, Value* args);
Value cosNative(int argCount, Value* args);
Value tanNative(int argCount, Value* args);
Value absNative(int argCount, Value* args);

#endif // FLS_STD_MATH_H
