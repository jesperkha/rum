#pragma once

#define TITLE "wim - v0.0.1"

#define RETURN_SUCCESS 0
#define RETURN_ERROR -1

#define BUFFER_LINE_CAP 4
#define DEFAULT_LINE_LENGTH 32

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
void editorWriteAt(int x, int y, const char *text);
int editorTerminalGetSize();
int editorHandleInput();

void screenBufferClearAll();
void screenBufferClearLine(int row);
void screenBufferWrite(const char *string, int length);

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
void bufferRenderLine(int row);
void bufferSplitLineDown(int row);
void bufferSplitLineUp(int row);
void bufferRenderLines();
