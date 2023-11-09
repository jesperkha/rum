#pragma once

#define TITLE "wim - v0.0.1"

#define RETURN_SUCCESS 0
#define RETURN_ERROR -1

#define DEFAULT_LINE_LENGTH 32

#define return_error(msg)    \
    {                        \
        error(msg);          \
        return RETURN_ERROR; \
    }

enum KeyCodes
{
    BACKSPACE = 8,
    TAB = 9,
    ENTER = 13,
    ESCAPE = 27,
    SPACE = 32,
    COLON = 58,

    ARROW_LEFT = 37,
    ARROW_UP,
    ARROW_RIGHT,
    ARROW_DOWN,
};

void editorInit();
void editorExit();
int editorClearScreen();
int editorTerminalGetSize();
int editorHandleInput();
void editorWriteAt(int x, int y, const char* text);

void cursorHide();
void cursorShow();
void cursorSetPos(int x, int y);
void cursorMove(int x, int y);
void cursorTempPos(int x, int y);
void cursorRestore();

void bufferCreate();
void bufferFree();
void bufferWriteChar(char c);
void bufferDeleteChar();
void bufferInsertLine(int row);
void bufferRenderLine(int row);