#include "rum.h"

StrArray StrArrayNew(int size)
{
    StrArray arr = {
        .cap = size,
        .length = 0,
        .ptr = MemZeroAlloc(size),
    };
    AssertNotNull(arr.ptr);
    return arr;
}

int StrArraySet(StrArray *a, char *source, int length)
{
    if (a->length + length + 1 >= a->cap)
    {
        a->ptr = MemRealloc(a->ptr, a->cap * 2);
        a->cap *= 2;
        AssertNotNull(a->ptr);
    }

    int idx = a->length;
    strncpy(a->ptr + a->length, source, length);
    a->ptr[a->length + length] = 0;
    a->length += length + 1;
    return idx;
}

char *StrArrayGet(StrArray *a, int idx)
{
    if (idx >= a->length)
        Panicf("String array get index %d of length %d", idx, a->length);

    return a->ptr + idx;
}

void StrArrayFree(StrArray *a)
{
    MemFree(a->ptr);
}