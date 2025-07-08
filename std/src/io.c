#include <stdio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "io.h"
#include "value.h"
#include "object.h" // For string objects
#include "vm.h"     // For runtimeError
#include <ctype.h>

// Helper to trim leading/trailing whitespace and quotes from a string, in-place.
static void trimString(char* str) {
    if (str == NULL) return;

    char* start = str;
    // Find the first non-whitespace, non-quote character
    while (isspace((unsigned char)*start) || *start == '\'' || *start == '"') {
        start++;
    }

    // Find the last non-whitespace, non-quote character
    char* end = str + strlen(str) - 1;
    while (end > start && (isspace((unsigned char)*end) || *end == '\'' || *end == '"')) {
        end--;
    }

    // Null-terminate the trimmed string
    *(end + 1) = '\0';

    // Shift the trimmed string to the beginning if necessary
    if (str != start) {
        memmove(str, start, end - start + 2); // +2 to include the null terminator
    }
}

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

    // Create a mutable copy of the path to trim it.
    const char* originalPath = AS_CSTRING(args[0]);
    char* path = (char*)malloc(strlen(originalPath) + 1);
    if (path == NULL) {
        runtimeError("Memory allocation failed.");
        return NIL_VAL;
    }
    strcpy(path, originalPath);
    trimString(path);

    size_t fileSize;
    char* content = readFileText(path, &fileSize);

    if (content == NULL) {
        runtimeError("Could not read file \"%s\".", path);
        free(path);
        return NIL_VAL;
    }

    free(path);
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

Value pathExistsNative(int argCount, Value* args) {
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
        runtimeError("isFile() expects one string argument.");
        return NIL_VAL;
    }
    const char* path = AS_CSTRING(args[0]);
    struct stat buffer;
    if (stat(path, &buffer) != 0) {
        return BOOL_VAL(false);
    }
    return BOOL_VAL(S_ISREG(buffer.st_mode));
}

Value createDirNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        runtimeError("createDir() expects one string argument (path).");
        return NIL_VAL;
    }
    const char* path = AS_CSTRING(args[0]);
    // mkdir returns 0 on success. We'll return true for success.
    // The second argument is the mode, 0777 is a common default.
    return BOOL_VAL(mkdir(path, 0777) == 0);
}

Value startsWithNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        runtimeError("startsWith() expects two string arguments.");
        return NIL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    ObjString* prefix = AS_STRING(args[1]);
    if (prefix->length > str->length) return BOOL_VAL(false);
    return BOOL_VAL(strncmp(str->chars, prefix->chars, prefix->length) == 0);
}

Value substringNative(int argCount, Value* args) {
    if (argCount != 3 || !IS_STRING(args[0]) || !IS_NUMBER(args[1]) || !IS_NUMBER(args[2])) {
        runtimeError("substring() expects a string and two numbers (start, end).");
        return NIL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    int start = AS_NUMBER(args[1]);
    int end = AS_NUMBER(args[2]);

    if (start < 0 || end > str->length || start > end) {
        runtimeError("Substring bounds are out of range.");
        return NIL_VAL;
    }

    int length = end - start;
    return OBJ_VAL(copyString(str->chars + start, length));
}

Value splitNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        runtimeError("split() expects two string arguments (string, delimiter).");
        return NIL_VAL;
    }

    ObjString* str = AS_STRING(args[0]);
    ObjString* delimiter = AS_STRING(args[1]);
    ObjList* list = newList();
    push(OBJ_VAL(list));

    const char* source = str->chars;
    const char* delim = delimiter->chars;
    int delim_len = delimiter->length;

    if (delim_len == 0) { // Handle empty delimiter
        // Just return the original string in a list
        writeValueArray(list->items, OBJ_VAL(copyString(source, str->length)));
        pop();
        return OBJ_VAL(list);
    }

    const char* current = source;
    const char* found = strstr(current, delim);

    while (found != NULL) {
        int token_len = found - current;
        Value tokenValue = OBJ_VAL(copyString(current, token_len));
        writeValueArray(list->items, tokenValue);

        current = found + delim_len;
        found = strstr(current, delim);
    }

    // Add the final part of the string after the last delimiter
    writeValueArray(list->items, OBJ_VAL(copyString(current, strlen(current))));

    pop();
    return OBJ_VAL(list);
}
