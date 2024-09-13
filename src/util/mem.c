#include "rum.h"

void *MemAlloc(int size)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}

void *MemZeroAlloc(int size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

void *MemRealloc(void *ptr, int newSize)
{
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ptr, newSize);
}

void MemFree(void *ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}
