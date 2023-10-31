#pragma once

#define RETURN_SUCCESS 0
#define RETURN_ERROR -1

#define DEFUALT_LINE_LENGTH 32

#define error(msg) printf("Error: %s\n", msg);
#define log(msg) printf("Log: %s\n", msg);
#define return_error(msg)    \
    {                        \
        error(msg);          \
        return RETURN_ERROR; \
    }

// Corresponds to a single line, or row, in the editor buffer
typedef struct linebuf
{
    int idx;      // Row index in file
    int cap;      // Capacity of line
    int size;     // Length of line
    int rsize;    // Length of rendered string
    char *chars;  // Characters in line
    char *render; // Points to beginning of rendered chars in chars
} linebuf;

enum KeyCodes
{
    BACKSPACE = 8,
    TAB = 9,
    ENTER = 13,
    ESCAPE = 27,
    SPACE = 32,
    COLON = 58,
};

int editorClearScreen();
int editorTerminalResize();

void bufferWriteChar(char c);
void bufferDeleteChar();
void bufferInsertLine(int row);
void bufferCreateEmpty(int n);
void bufferFree();
void bufferRenderLine(linebuf line);