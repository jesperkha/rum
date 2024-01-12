#pragma once

void Log(char *message);
void LogError(char *message);
void LogNumber(char *message, int number);

#define check_pointer(ptr, where) \
    if (ptr == NULL)              \
        LogError("null pointer:  " where);

void *memAlloc(int size);
void *memZeroAlloc(int size);
void *memRealloc(void *ptr, int newSize);
void memFree(void *ptr);