#pragma once

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>

#define TITLE "wim - v0.0.1"

#define RETURN_SUCCESS 0
#define RETURN_ERROR -1

#define BUFFER_LINE_CAP 32
#define DEFAULT_LINE_LENGTH 32

#define return_error(msg) return RETURN_ERROR;

#define cursor_real_y (editor.row - editor.offy)
#define cursor_real_x (editor.col - editor.offx)

enum KeyCodes
{
    K_BACKSPACE = 8,
    K_TAB = 9,
    K_ENTER = 13,
    K_ESCAPE = 27,
    K_SPACE = 32,
    K_PAGEUP = 33,
    K_PAGEDOWN = 34,
    K_DELETE = 46,
    K_COLON = 58,

    K_ARROW_LEFT = 37,
    K_ARROW_UP,
    K_ARROW_RIGHT,
    K_ARROW_DOWN,
};

#define COL_RESET "\033[0m"
#define COL_BG_WHITE "\033[47m"

void editorInit();
void editorExit();
void editorWriteAt(int x, int y, const char *text);
int editorTerminalGetSize();
int editorHandleInput();
int editorLoadFile(const char *filepath);

void screenBufferWrite(const char *string, int length);
void screenBufferClearAll();
void screenBufferClearLine(int row);

void cursorHide();
void cursorShow();
void cursorSetPos(int x, int y);
void cursorMove(int x, int y);
void cursorTempPos(int x, int y);
void cursorRestore();

void bufferCreate();
void bufferFree();
void bufferCreateLine(int idx);
void bufferWriteChar(char c);
void bufferDeleteChar();
void bufferExtendLine(int row, int new_size);
void bufferInsertLine(int row);
void bufferDeleteLine(int row);
void bufferSplitLineDown(int row);
void bufferSplitLineUp(int row);
void bufferScroll(int n);

void renderBuffer();
void renderSatusBar();
