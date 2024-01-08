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


enum
{
    UI_YES,
    UI_NO,
    UI_OK,
    UI_CANCEL,
};

int uiPromptYesNo(char *message, bool select);
int uiTextInput(int x, int y, char *buffer, int size);

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