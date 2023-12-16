#pragma once

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>

#define TITLE "wim - v0.0.1"

#define RETURN_SUCCESS 1
#define RETURN_ERROR 0

#define BUFFER_LINE_CAP 32
#define DEFAULT_LINE_LENGTH 256

typedef struct Line
{
    int idx; // Row index in file, not buffer
    int row; // Relative row in buffer, not file

    int cap;     // Capacity of line
    int length;  // Length of line
    char *chars; // Characters in line
} Line;

typedef struct Info
{
    char filename[64];
    char error[64];
    bool hasError;
    bool dirty;
    bool fileOpen;
} Info;

typedef struct Config
{
    bool syntaxEnabled;
    bool matchParen;
} Config;

typedef struct Editor
{
    Info info;
    Config config;

    HANDLE hstdin;  // Handle for standard input
    HANDLE hbuffer; // Handle to new screen buffer

    COORD initSize;    // Inital size to return to
    int width, height; // Size of terminal window
    int textW, textH;  // Size of text editing area
    int padV, padH;    // Vertical and horizontal padding

    int row, col;           // Current row and col of cursor in buffer
    int colMax;             // Keep track of the most right pos of the cursor when moving down
    int offx, offy;         // x, y offset from left/top
    int indent;             // Indent in spaces for current line
    int scrollDx, scrollDy; // Minimum distance from top/bottom or left/right before scrolling

    int numLines, lineCap; // Count and capacity of lines in array
    Line *lines;           // Array of lines in buffer

    char *renderBuffer; // Written to and printed on render
} Editor;

enum KeyCodes
{
    K_BACKSPACE = 8,
    K_TAB = 9,
    K_ENTER = 13,
    K_CTRL = 17,
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

Editor *editorGetHandle();
void editorInit();
void editorExit();
void editorWriteAt(int x, int y, const char *text);
void editorUpdateSize();
int editorHandleInput();
int editorOpenFile(char *filepath);
int editorSaveFile(char *filepath);
void editorCommand();

void screenBufferWrite(const char *string, int length);
void screenBufferClearAll();
void screenBufferClearLine(int row);

void cursorHide();
void cursorShow();
void cursorSetPos(int x, int y, bool keepX);
void cursorMove(int x, int y);
void cursorTempPos(int x, int y);
void cursorRestore();

void bufferCreateLine(int idx);
void bufferWriteChar(char c);
void bufferDeleteChar();
void bufferExtendLine(int row, int new_size);
void bufferInsertLine(int row);
void bufferDeleteLine(int row);
void bufferSplitLineDown(int row);
void bufferSplitLineUp(int row);
void bufferScroll(int x, int y);
void bufferScrollDown();
void bufferScrollUp();

void typingDeleteForward();
void typingBreakParen();
void typingMatchParen(char c);

void renderBuffer();
void renderBufferBlank();

void statusBarUpdate(char *filename, char *error);

typedef struct CharBuffer
{
    char *buffer;
    char *pos;
    int lineLength;
} CharBuffer;

void charbufAppend(CharBuffer *buf, char *src, int length);
void charbufNextLine(CharBuffer *buf);
void charbufColor(CharBuffer *buf, char *col);
void charbufRender(CharBuffer *buf, int x, int y);

enum
{
    UI_YES,
    UI_NO,
    UI_OK,
    UI_CANCEL,
};

int uiPromptYesNo(const char *message);
int uiTextInput(int x, int y, char *buffer, int size);

enum
{
    HL_KEYWORD,
    HL_NUMBER,
    HL_STRING,
    HL_TYPE,
};

// \x1b[38;2;r;g;bm - foreground
// \x1b[48;2;r;g;bm - background
// https://github.com/morhetz/gruvbox

#define BG(col) "\x1b[48;2;" col "m"
#define FG(col) "\x1b[38;2;" col "m"

#define COL_RESET "\x1b[0m"
#define COL_BG0 "40;40;40"
#define COL_BG1 "60;56;54"
#define COL_BG2 "80;73;69"
#define COL_BG3 "102;92;83"
#define COL_FG0 "235;219;178"
#define COL_YELLOW "215;153;33"

#define COL_RED "251;73;52"
#define COL_ORANGE "254;128;25"
#define COL_BLUE "131;165;152"
#define COL_GREY "146;131;116"
#define COL_AQUA "142;192;124"
#define COL_PINK "211;134;155"
#define COL_GREEN "185;187;38"

char *highlightLine(char *line, int length, int *newLength);
