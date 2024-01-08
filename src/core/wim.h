#pragma once

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>

#include "buffer.h"
#include "editor.h"

#define TITLE "wim v0.1.0"
#define UPDATED "21.12.23"

#define RETURN_SUCCESS 1
#define RETURN_ERROR 0

#define BUFFER_LINE_CAP 32     // Editor line array cap
#define DEFAULT_LINE_LENGTH 32 // Length of line char array

#define THEME_NAME_LEN 32  // Length of name in theme file

// File types for highlighting
enum FileTypes
{
    FT_UNKNOWN,
    FT_C,
    FT_PYTHON,
};


void screenBufferWrite(const char *string, int length);
void screenBufferClearAll();
void screenBufferBg(int col);
void screenBufferFg(int col);
void screenBufferClearLine(int row);

void cursorHide();
void cursorShow();
void cursorSetPos(int x, int y, bool keepX);
void cursorMove(int x, int y);
void cursorTempPos(int x, int y);
void cursorRestore();

void typingInsertTab();
void typingDeleteForward();
void typingBreakParen();
void typingMatchParen(char c);

void renderBuffer();
void renderBufferBlank();

void statusBarUpdate(char *filename, char *error);
void statusBarClear();

typedef struct CharBuffer
{
    char *buffer;
    char *pos;
    int lineLength;
} CharBuffer;

void charbufClear(CharBuffer *buf);
void charbufAppend(CharBuffer *buf, char *src, int length);
void charbufNextLine(CharBuffer *buf);
void charbufColor(CharBuffer *buf, char *col);
void charbufRender(CharBuffer *buf, int x, int y);
void charbufBg(CharBuffer *buf, int col);
void charbufFg(CharBuffer *buf, int col);

enum
{
    UI_YES,
    UI_NO,
    UI_OK,
    UI_CANCEL,
};

int uiPromptYesNo(char *message, bool select);
int uiTextInput(int x, int y, char *buffer, int size);

// Syntax highlighting and ui color functions

enum
{
    HL_KEYWORD,
    HL_NUMBER,
    HL_STRING,
    HL_TYPE,
};

#define COL_RESET "\x1b[0m"

#define COL_BG0 (12 * 0)    // Editor background
#define COL_BG1 (12 * 1)    // Statusbar and current line bg
#define COL_BG2 (12 * 2)    // Comments, line numbers
#define COL_FG0 (12 * 3)    // Text
#define COL_YELLOW (12 * 4) // Function name
#define COL_BLUE (12 * 5)   // Object
#define COL_PINK (12 * 6)   // Number
#define COL_GREEN (12 * 7)  // String, char
#define COL_AQUA (12 * 8)   // Math symbol, macro
#define COL_ORANGE (12 * 9) // Type name
#define COL_RED (12 * 10)   // Keyword
#define COL_GREY (12 * 11)  // Other symbol

char *highlightLine(char *line, int lineLength, int *newLength);

// Logging and debug info

void Log(char *message);
void LogError(char *message);
void LogNumber(char *message, int number);

#define check_pointer(ptr, where) \
    if (ptr == NULL) LogError("null pointer:  "where);

// Memory allocation

void *memAlloc(int size);
void *memZeroAlloc(int size);
void *memRealloc(void *ptr, int newSize);
void memFree(void *ptr);