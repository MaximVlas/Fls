#include <stdio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "io.h"
#include "value.h"
#include "object.h" // For string objects
#include "vm.h"     // For runtimeError

Value printNative(int argCount, Value* args) {
    for (int i = 0; i < argCount; i++) {
        printValue(args[i]);
        if (i < argCount - 1) {
            printf(" ");
        }
    }
    return NIL_VAL;
}

Value printlnNative(int argCount, Value* args) {
    printNative(argCount, args);
    printf("\n");
    return NIL_VAL;
}

// Helper to read a file into a string
static char* readFileText(const char* path, size_t* fileSize) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    *fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(*fileSize + 1);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    size_t bytesRead = fread(buffer, sizeof(char), *fileSize, file);
    if (bytesRead < *fileSize) {
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[bytesRead] = '\0';
    fclose(file);
    return buffer;
}

Value readFileNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        runtimeError("readFile() takes one string argument (path).");
        return NIL_VAL;
    }
    const char* path = AS_CSTRING(args[0]);
    size_t fileSize;
    char* content = readFileText(path, &fileSize);

    if (content == NULL) {
        runtimeError("Could not read file \"%s\".", path);
        return NIL_VAL;
    }

    return OBJ_VAL(takeString(content, fileSize));
}

Value writeFileNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        runtimeError("writeFile() takes two string arguments (path, content).");
        return NIL_VAL;
    }
    const char* path = AS_CSTRING(args[0]);
    const char* content = AS_CSTRING(args[1]);

    FILE* file = fopen(path, "wb");
    if (file == NULL) {
        runtimeError("Could not open file \"%s\" for writing.", path);
        return NIL_VAL;
    }

    size_t contentLength = AS_STRING(args[1])->length;
    fwrite(content, sizeof(char), contentLength, file);
    fclose(file);

    return NIL_VAL;
}

Value appendFileNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        runtimeError("appendFile() takes two string arguments (path, content).");
        return NIL_VAL;
    }
    const char* path = AS_CSTRING(args[0]);
    const char* content = AS_CSTRING(args[1]);

    FILE* file = fopen(path, "ab");
    if (file == NULL) {
        runtimeError("Could not open file \"%s\" for appending.", path);
        return NIL_VAL;
    }

    size_t contentLength = AS_STRING(args[1])->length;
    fwrite(content, sizeof(char), contentLength, file);
    fclose(file);

    return NIL_VAL;
}

Value fileExistsNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        runtimeError("fileExists() takes one string argument (path).");
        return NIL_VAL;
    }
    const char* path = AS_CSTRING(args[0]);
    struct stat buffer;
    return BOOL_VAL(stat(path, &buffer) == 0);
}

Value deleteFileNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        runtimeError("deleteFile() takes one string argument (path).");
        return NIL_VAL;
    }
    const char* path = AS_CSTRING(args[0]);
    return BOOL_VAL(remove(path) == 0);
}

Value fileSizeNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        runtimeError("fileSize() takes one string argument (path).");
        return NIL_VAL;
    }
    const char* path = AS_CSTRING(args[0]);
    struct stat buffer;
    if (stat(path, &buffer) != 0) {
        return NIL_VAL;
    }
    return NUMBER_VAL(buffer.st_size);
}

Value isDirNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        runtimeError("isDir() takes one string argument (path).");
        return NIL_VAL;
    }
    const char* path = AS_CSTRING(args[0]);
    struct stat buffer;
    if (stat(path, &buffer) != 0) {
        return BOOL_VAL(false);
    }
    return BOOL_VAL(S_ISDIR(buffer.st_mode));
}

Value isFileNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        runtimeError("isFile() takes one string argument (path).");
        return NIL_VAL;
    }
    const char* path = AS_CSTRING(args[0]);
    struct stat buffer;
    if (stat(path, &buffer) != 0) {
        return BOOL_VAL(false);
    }
    return BOOL_VAL(S_ISREG(buffer.st_mode));
}
