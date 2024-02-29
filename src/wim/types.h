#pragma once

#include "wim.h"

// Editor configuration loaded from config file. Editor must be reloaded for all
// changes to take effect. Config is global and affects all buffers.
typedef struct Config
{
    bool syntaxEnabled; // Enable syntax highlighting for some files
    bool matchParen;    // Match ending parens when typing. eg: '(' adds a ')'
    bool useCRLF;       // Use CRLF line endings. (NOT IMPLEMENTED)
    byte tabSize;       // Amount of spaces a tab equals
} Config;

// Each buffer has a cursor object attached to it. The cursor essentially holds
// a pointer to the text to be edited in the Buffer as well as were to place the
// cursor on-screen.
typedef struct Cursor
{
    int row, col;   // Row/col in raw text buffer
    int colMax;     // Furthest right position while moving up/down
    int indent;     // Of current line
    int offx, offy; // Inital offset from left/top
    int scrollDx;   // Minimum distance before scrolling right
    int scrollDy;   // Minimum distance before scrolling up/down
} Cursor;

#define LINE_DEFAULT_LENGTH 32

// Line in buffer. Holds raw text.
typedef struct Line
{
    int row;
    int cap;
    int length;
    char *chars;
} Line;

#define BUFFER_DEFAULT_LINE_CAP 32

// A buffer holds text, usually a file, and is editable.
typedef struct Buffer
{
    Cursor cursor;

    bool isFile;        // Does the buffer contain a file?
    bool dirty;         // Has the buffer changed since last save?
    bool syntaxReady;   // Is syntax highlighting available for this file?
    char filepath[260]; // Full path to file

    int textH;
    int padX, padY; // Padding on left and top of text area

    int numLines;
    int lineCap;
    Line *lines;
} Buffer;

#define EDITOR_BUFFER_CAP 16

// // The Editor contains the buffers and the current state of the editor.
// typedef struct Editor
// {
//     int width, height; // Total size of editor

//     HANDLE hbuffer; // Handle for created screen buffer
//     HANDLE hstdout; // NOT USED
//     HANDLE hstdin;  // Console input

//     int numBuffers;
//     int activeBuffer;
//     Buffer buffers[EDITOR_BUFFER_CAP];

//     EditorAction *actions;
//     char renderBuffer[]
// } Editor;