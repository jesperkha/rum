#include "wim.h"

typedef enum Type
{
    T_LIST,
} Type;

typedef struct TypeHeader
{
    Type type;

    int length;
    int cap;
    int dataSize;
} TypeHeader;

#define HEADER_SIZE (sizeof(TypeHeader))

static TypeHeader *getHeader(void *ptr)
{
    return (TypeHeader*)(ptr - HEADER_SIZE);
}

// Implementation of 'dynamic array'. Allows direct index access while
// also doing reallocation and stuff in the background.

// Returns pointer to new List
void *ListCreate(size_t dataSize, size_t length)
{
    TypeHeader header = {
        .type = T_LIST,
        .length = 0,
        .cap = length,
        .dataSize = dataSize,
    };

    void *ptr = memZeroAlloc(HEADER_SIZE + (dataSize * length));
    memcpy(ptr, &header, HEADER_SIZE);
    return (ptr + HEADER_SIZE);
}

// Returns length of list. Only valid if appended to with ListAppend or append.
int ListLen(void *list)
{
    return getHeader(list)->length;
}

// Appends item to end of list. Fails if full.
void ListAppend(void *list, superlong item)
{
    TypeHeader *header = getHeader(list);
    int size = header->dataSize;

    if (header->length < header->cap)
    {
        int length = (header->length * size);

        if (size > sizeof(superlong))
            memcpy(list + length, (void*)item, size);
        else
            memcpy(list + length, &item, size);

        header->length++;
    }
}

void *ListPop(void *list)
{
    TypeHeader *header = getHeader(list);
    header->length--;
    if (header->length < 0) // Debug
        LogError("list pop with length 0");
    return list + (header->length * header->dataSize);
}

// Frees array and type header.
void ListFree(void *list)
{
    memFree(list - HEADER_SIZE);
}