#pragma once

#include "list.h"

#ifdef DEBUG

void Log(char *message);
void LogError(char *message);
void LogNumber(char *message, int number);

#define check_pointer(ptr, where) \
    if (ptr == NULL)              \
        LogError("null pointer:  " where);

#else

#define Log(...)
#define LogNumber(...)
#define LogError(...)
#define check_pointer(...)

#endif

void *MemAlloc(int size);
void *MemZeroAlloc(int size);
void *MemRealloc(void *ptr, int newSize);
void MemFree(void *ptr);

// Gets filename, including extension, from filepath
void StrFilename(char *dest, char *src);

// Gets the file extension, excluding the peroid.
void StrFileExtension(char *dest, char *src);

// Returns pointer to first character in first instance of substr in buf. NULL if none is found.
char *StrMemStr(char *buf, char *substr, size_t size);