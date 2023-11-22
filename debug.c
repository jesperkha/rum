#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>

#include "wim.h"

void logError(const char *msg);
void logNumber(const char *what, int number);

void *__calloc(size_t count, size_t size);
void *__realloc(void *ptr, size_t newsize);
void *__malloc(size_t size);
void __free(void *ptr);

#define calloc __calloc
#define malloc __malloc
#define realloc __realloc
#define free __free

#undef return_error
#define return_error(msg)    \
    {                        \
        logError(msg);       \
        return RETURN_ERROR; \
    }

#define check_pointer(ptr, where)       \
    if (ptr == NULL)                    \
    {                                   \
        logError("NULL pointer alloc"); \
        logError(where);                \
        exit(1);                        \
    }

void logNumber(const char *what, int number)
{
    FILE *f = fopen("log", "a");
    check_pointer(f, "logToFile");
    fprintf(f, "[ LOG ]: %s, %d\n", what, number);
    fclose(f);
}

void logError(const char *msg)
{
    FILE *f = fopen("log", "a");
    check_pointer(f, "logError");
    fprintf(f, "[ ERROR ]: %s, Windows error code: %d\n", msg, (int)GetLastError());
    fclose(f);
}

void *__calloc(size_t count, size_t size)
{
    logNumber("Calloc", count * size);
    void *mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, count * size);
    if (mem == NULL)
    {
        logError("Calloc failed");
        editorExit();
    }

    return mem;
}

void *__malloc(size_t size)
{
    logNumber("Malloc", size);
    void *mem = HeapAlloc(GetProcessHeap(), 0, size);
    if (mem == NULL)
    {
        logError("Malloc failed");
        editorExit();
    }

    return mem;
}

void *__realloc(void *ptr, size_t newsize)
{
    logNumber("Realloc", newsize);
    void *mem = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ptr, newsize);
    if (mem == NULL)
    {
        logError("Realloc failed");
        editorExit();
    }

    return mem;
}

void __free(void *ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}
