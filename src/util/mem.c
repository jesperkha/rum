#include "wim.h"

void *memAlloc(int size)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}

void *memZeroAlloc(int size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

void *memRealloc(void *ptr, int newSize)
{
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ptr, newSize);
}

void memFree(void *ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}