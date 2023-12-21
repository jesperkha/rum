#pragma once

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

#undef check_pointer
#define check_pointer(ptr, where)       \
    if (ptr == NULL)                    \
    {                                   \
        logError("NULL pointer alloc"); \
        logError(where);                \
        exit(1);                        \
    }
