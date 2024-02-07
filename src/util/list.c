#include "list.h"

// Implementation of 'dynamic array', or list. Allows direct index access while
// also doing reallocation and stuff in the background.

typedef struct ListHeader
{
    int length;
    int cap;
    int dataSize; // sizeof(type)
} ListHeader;

#define HEADER_SIZE (sizeof(ListHeader))

static ListHeader *getHeader(void *ptr)
{
    return (ListHeader *)(ptr - HEADER_SIZE);
}

void *ListCreate(size_t dataSize, size_t length)
{
    ListHeader header = {
        .length = 0,
        .cap = length,
        .dataSize = dataSize,
    };

    void *listptr = calloc(1, HEADER_SIZE + (dataSize * length));
    memcpy(listptr, &header, HEADER_SIZE);
    return (listptr + HEADER_SIZE);
}

// Returns length of list. Only valid if appended to with ListAppend or append.
int ListLen(void *list)
{
    return getHeader(list)->length;
}

// Return capacity of list, given in declaration.
int ListCap(void *list)
{
    return getHeader(list)->cap;
}

// Appends item to end of list. Fails if full.
void ListAppend(void *list, uint64_t item)
{
    ListHeader *header = getHeader(list);
    int size = header->dataSize;

    if (header->length < header->cap)
    {
        int length = (header->length * size);

        if (size > sizeof(uint64_t))
            memcpy(list + length, (void *)item, size);
        else
            memcpy(list + length, &item, size);

        header->length++;
    }
}

// Removes and returns last element in list.
void *ListPop(void *list)
{
    ListHeader *header = getHeader(list);
    header->length--;
    return list + (header->length * header->dataSize);
}

void ListFree(void *list)
{
    free(list - HEADER_SIZE);
}