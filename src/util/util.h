#pragma once

#include "list.h"

void *MemAlloc(int size);
void *MemZeroAlloc(int size);
void *MemRealloc(void *ptr, int newSize);
void MemFree(void *ptr);

// Gets filename, including extension, from filepath
void StrFilename(char *dest, char *src);

// Gets the file extension, excluding the peroid.
void StrFileExtension(char *dest, char *src);

// Returns pointer to first character in first instance of substr in buf. NULL if none is found.
char *StrMemStr(char *buf, char *substr, size_t size);

// Used to store text before rendering.
typedef struct CharBuf
{
    char *buffer;
    char *pos;
    int lineLength;
} CharBuf;

// Returns pointer to empty CharBuf mapped to input buffer.
CharBuf *CbNew(char *buffer);

// Resets buffer to starting state. Does not memclear the internal buffer.
void CbReset(CharBuf *buf);

void CbAppend(CharBuf *buf, char *src, int length);

// Fills remaining line with space characters based on editor width.
void CbNextLine(CharBuf *buf);

// Adds background and foreground color to buffer.
void CbColor(CharBuf *buf, char *bg, char *fg);
void CbBg(CharBuf *buf, char *bg);
void CbFg(CharBuf *buf, char *fg);

// Adds COL_RESET to buffer
void CbColorReset(CharBuf *buf);

// Prints buffer at x, y with accumulated length only.
void CbRender(CharBuf *buf, int x, int y);

#ifdef DEBUG

void _Log(char *message, char *filepath, int lineNumber);
void _LogError(char *message, char *filepath, int lineNumber);
void _LogNumber(char *message, int number, char *filepath, int lineNumber);
void _LogString(char *message, char *str, char *filepath, int lineNumber);
void LogCreate();

#define Log(msg) _Log(msg, __FILE__, __LINE__)
#define LogNumber(msg, number) _LogNumber(msg, number, __FILE__, __LINE__)
#define LogError(msg) _LogError(msg, __FILE__, __LINE__)
#define LogString(msg, str) _LogString(msg, str, __FILE__, __LINE__)

#define check_pointer(ptr, where) \
    if (ptr == NULL)              \
        LogError("null pointer:  " where);

#else

#define Log(message)
#define LogNumber(message, number)
#define LogError(message)
#define LogString(message, string)
#define LogCreate()
#define check_pointer(ptr, where)

#endif