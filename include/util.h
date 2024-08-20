#pragma once

#include "list.h"

#define clamp(MIN, MAX, v) (max(min((v), (MAX)), (MIN)))

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
// Returns true if c is a printable ascii character
bool isChar(char c);

// Used to store text before rendering.
typedef struct CharBuf
{
    char *buffer;
    char *pos;
    int lineLength;
} CharBuf;

// Returns empty CharBuf mapped to input buffer.
CharBuf CbNew(char *buffer);
// Resets buffer to starting state. Does not memclear the internal buffer.
void CbReset(CharBuf *buf);
void CbAppend(CharBuf *buf, char *src, int length);
void CbRepeat(CharBuf *buf, char c, int count);
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
