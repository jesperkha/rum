#include <windows.h>
#include <stdio.h>
#include <stdbool.h>

#include "wim.h"

// ---------------------- GLOBAL STATE ----------------------

struct editorGlobals
{
    HANDLE hstdin;  // Handle for standard input
    HANDLE hstdout; // Handle for standard output
    DWORD mode_in;  // Restore to previous input mode

    int width, height; // Size of terminal window
} editor;

struct editorFileBuffer
{
    int cx, cy;            // Cursor x, y
    int numLines, lineCap; // Size and capacity of line array
    linebuf *lines;        // Array of line buffers
} buffer;

// ---------------------- EDITOR ----------------------

// Populates editor global struct and creates empty file buffer. Exits on error.
void editorInit()
{
    editor.hstdin = GetStdHandle(STD_INPUT_HANDLE);
    editor.hstdout = GetStdHandle(STD_OUTPUT_HANDLE);

    GetConsoleMode(editor.hstdin, &editor.mode_in);
    SetConsoleMode(editor.hstdin, 0);

    if (editor.hstdin == NULL || editor.hstdout == NULL)
    {
        error("editorInit() - Failed to get std handles");
        exit(1);
    }

    if (editorTerminalResize() == RETURN_ERROR)
    {
        error("editorInit() - Failed to get window size");
        exit(1);
    }

    bufferCreateEmpty(16);
}

void editorExit()
{
    bufferFree();
    editorClearScreen();
    SetConsoleMode(editor.hstdin, editor.mode_in);
    exit(0);
}

void editorSetCursorPos()
{
    COORD pos = {buffer.cx, buffer.cy};
    SetConsoleCursorPosition(editor.hstdout, pos);
}

void editorHideCursor()
{
    CONSOLE_CURSOR_INFO info = {100, false};
    SetConsoleCursorInfo(editor.hstdout, &info);
}

void editorShowCursor()
{
    CONSOLE_CURSOR_INFO info = {100, true};
    SetConsoleCursorInfo(editor.hstdout, &info);
}

// Update editor size values. Returns -1 on error.
int editorTerminalResize()
{
    CONSOLE_SCREEN_BUFFER_INFO cinfo;
    if (!GetConsoleScreenBufferInfo(editor.hstdout, &cinfo))
        return_error("windowResize() - Failed to get buffer info");

    editor.width = (int)cinfo.srWindow.Right;
    editor.height = (int)(cinfo.srWindow.Bottom + 1);

    return RETURN_SUCCESS;
}

// Takes action based on current user input. Returns -1 on error.
int editorHandleInput()
{
    char inputChar;
    DWORD read;
    if (!ReadFile(editor.hstdin, &inputChar, 2, &read, NULL) || read == 0)
        return_error("editorHandleInput() - Failed to read input");

    switch (inputChar)
    {
    case 'q':
        editorExit();

    case ESCAPE:
        break;

    case BACKSPACE:
        bufferDeleteChar();
        break;

    case ENTER:
        bufferInsertLine(-1);
        buffer.cy++;
        buffer.cx = 0;
        bufferRenderLine(buffer.lines[buffer.cy]);
        break;

    default:
        // Assume normal character input
        bufferWriteChar(inputChar);
    }

    return RETURN_SUCCESS;
}

// Clears the terminal. Returns -1 on error.
int editorClearScreen()
{
    CONSOLE_SCREEN_BUFFER_INFO cinfo;
    if (!GetConsoleScreenBufferInfo(editor.hstdout, &cinfo))
        return_error("editorClearScreen() - Failed to get buffer info");

    COORD origin = {0, 0};
    SetConsoleCursorPosition(editor.hstdout, origin);

    DWORD written;
    DWORD size = editor.width * editor.height;
    if (!FillConsoleOutputCharacter(editor.hstdout, (WCHAR)' ', size, origin, &written))
        return_error("editorClearScreen() - Failed to fill buffer");

    return RETURN_SUCCESS;
}

// ---------------------- BUFFER ----------------------

void bufferRenderLine(linebuf line)
{
    editorHideCursor();
    COORD pos = {0, buffer.cy};
    SetConsoleCursorPosition(editor.hstdout, pos);
    printf("                                  ");
    SetConsoleCursorPosition(editor.hstdout, pos);
    printf("%s", line.chars);
    editorSetCursorPos();
    editorShowCursor();
}

// Write single character to current line.
void bufferWriteChar(char c)
{
    linebuf line = buffer.lines[buffer.cy];

    if (buffer.cx < line.size) // Shift right side of line
    {
        char *pos = line.chars + buffer.cx;
        memmove(pos + 1, pos, line.size - buffer.cx);
    }

    line.chars[buffer.cx++] = c;
    line.size++;
    bufferRenderLine(line);
}

// Deletes the caharcter before the cursor position.
void bufferDeleteChar()
{
    linebuf line = buffer.lines[buffer.cy];

    if (buffer.cx == 0)
        return;

    line.chars[--buffer.cx] = 0;
    line.size--;
    bufferRenderLine(line);
}

// Inserts new line at row. If row is -1 line is appended to end of file.
void bufferInsertLine(int row)
{
    // Append to end of file
    if (row == -1)
    {
        if (buffer.lineCap == buffer.numLines + 1)
        {
            error("editorInsertNewLine() - Realloc line array in file buffer");
            return;
        }

        int length = DEFUALT_LINE_LENGTH;
        char *chars = calloc(length, sizeof(char));
        chars[length - 1] = '|'; // Mark end of line for debug purposes

        buffer.lines[buffer.numLines++] = (linebuf){
            .chars = chars,
            .render = chars,
            .cap = length,
            .size = 0,
        };

        return;
    }

    // Insert at row index
    error("bufferInsertLine() - Insert at not implemented");
}

void bufferFree()
{
    for (int i = 0; i < buffer.numLines; i++)
        free(buffer.lines[i].chars);
}

// Creates an empty file buffer with line cap n.
void bufferCreateEmpty(int n)
{
    buffer.lineCap = n;
    buffer.numLines = 0;
    buffer.lines = calloc(n, sizeof(linebuf));
    bufferInsertLine(-1);
}

int main(void)
{
    editorInit();
    editorClearScreen();
    editorTerminalResize(); // Get new size of buffer after clear

    while (1)
    {
        editorHandleInput();
    }

    bufferFree();
    editorClearScreen();

    return 0;
}