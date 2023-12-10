#include "util.h"

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
    // logNumber("Calloc", count * size);
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
    // logNumber("Malloc", size);
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
    // logNumber("Realloc", newsize);
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
    if (!HeapFree(GetProcessHeap(), 0, ptr))
        logError("Free failed");
}
