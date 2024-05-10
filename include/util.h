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
