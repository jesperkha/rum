#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Returns pointer to new list
void *ListCreate(size_t dataSize, size_t length);
void ListFree(void *list);
// Returns length of list. Only valid if appended to with ListAppend or append.
int ListLen(void *list);
// Return capacity of list, given in declaration.
int ListCap(void *list);
// Appends item to end of list. Reallocs if full
void ListAppend(void *list, uint64_t item);
// Removes and returns last element in list.
void *ListPop(void *list);

#define list(T, size) (T *)ListCreate(sizeof(T), size)
#define append(list, item) ListAppend((list), (uint64_t)item)
#define len(list) (ListLen(list))
#define cap(list) (ListCap(list))
#define pop(list) (ListPop(list))