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

#define superlong unsigned long long int

void *ListCreate(size_t dataSize, size_t length);
int ListLen(void *list);
void ListAppend(void *list, superlong item);
void ListFree(void *list);

#define List(T, size) (T*)ListCreate(sizeof(T), size)
#define len(list) (ListLen(list))
#define append(list, item) ListAppend(list, (superlong)item)