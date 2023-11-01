#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

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
        ExitProcess(EXIT_FAILURE);
    }

    editorTerminalGetSize();
    if (editorClearScreen() == RETURN_ERROR)
    {
        error("editorInit() - Failed to clear screen");
        ExitProcess(EXIT_FAILURE);
    }

    editorTerminalGetSize(); // Get new height of buffer
    bufferCreateEmpty(16);
    FlushConsoleInputBuffer(editor.hstdin);
    SetConsoleTitle(TITLE);
}

// Free, clean, and exit
void editorExit()
{
    bufferFree();
    editorClearScreen();
    SetConsoleMode(editor.hstdin, editor.mode_in);
    ExitProcess(EXIT_SUCCESS);
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
int editorTerminalGetSize()
{
    CONSOLE_SCREEN_BUFFER_INFO cinfo;
    if (!GetConsoleScreenBufferInfo(editor.hstdout, &cinfo))
        return_error("editorTerminalGetSize() - Failed to get buffer info");

    editor.width = (int)cinfo.srWindow.Right;
    editor.height = (int)(cinfo.srWindow.Bottom + 1);

    return RETURN_SUCCESS;
}

// Takes action based on current user input. Returns -1 on error.
int editorHandleInput()
{
    INPUT_RECORD record;
    DWORD read;

    if (!ReadConsoleInput(editor.hstdin, &record, 1, &read) || read == 0)
        return_error("editorHandleInput() - Failed to read input");

    if (record.EventType == KEY_EVENT)
    {
        if (record.Event.KeyEvent.bKeyDown)
        {
            KEY_EVENT_RECORD event = record.Event.KeyEvent;
            WORD keyCode = event.wVirtualKeyCode;
            char inputChar = event.uChar.AsciiChar;

            switch (keyCode)
            {
            case ESCAPE:
                editorExit();

            case BACKSPACE: // Delete char
                bufferDeleteChar();
                break;

            case ENTER: // Insert new line
                bufferInsertLine(-1);
                buffer.cy++;
                buffer.cx = 0;
                bufferRenderLine(&buffer.lines[buffer.cy]);
                break;

            case ARROW_LEFT: // Cursor left
                buffer.cx--;
                if (buffer.cx < 0)
                    buffer.cx = 0;
                editorSetCursorPos();
                break;

            case ARROW_RIGHT: // Cursor right
                buffer.cx++;
                linebuf *line = &buffer.lines[buffer.cy];
                if (buffer.cx > line->length)
                    buffer.cx = line->length;
                editorSetCursorPos();
                break;

            default: // Write char if valid
                if (inputChar >= 32 && inputChar < 127)
                    bufferWriteChar(inputChar);
            }
        }
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

void bufferRenderLine(linebuf *line)
{
    editorHideCursor();
    COORD pos = {0, buffer.cy};
    SetConsoleCursorPosition(editor.hstdout, pos);
    for (int i = 0; i < line->cap; i++)
        printf(" ");
    printf("| %d/%d ", line->length, line->cap);
    SetConsoleCursorPosition(editor.hstdout, pos);
    printf("%s", line->chars);
    editorSetCursorPos();
    editorShowCursor();
}

// Write single character to current line.
void bufferWriteChar(char c)
{
    linebuf *line = &buffer.lines[buffer.cy];
    if (buffer.cx + 1 >= line->cap)
    {
        // Extend line
        return;
    }

    // Shift line to the right
    if (buffer.cx < line->length)
    {
        char *pos = line->chars + buffer.cx;
        memmove(pos + 1, pos, line->length - buffer.cx);
    }

    line->chars[buffer.cx++] = c;
    line->length++;
    bufferRenderLine(line);
}

// Deletes the caharcter before the cursor position.
void bufferDeleteChar()
{
    linebuf *line = &buffer.lines[buffer.cy];

    if (buffer.cx == 0)
        return;

    line->chars[--buffer.cx] = 0;
    line->length--;

    // Shift line to the left
    if (buffer.cx < line->length)
    {
        char *pos = line->chars + buffer.cx;
        memmove(pos, pos + 1, line->length - buffer.cx);
        line->chars[line->length] = 0; // Set new line end
    }

    bufferRenderLine(line);
}

// Inserts new line at row. If row is -1 line is appended to end of file.
void bufferInsertLine(int row)
{
    if (buffer.lineCap == buffer.numLines + 1)
    {
        error("editorInsertNewLine() - Realloc line array in file buffer");
        return;
    }

    int length = DEFUALT_LINE_LENGTH;
    char *chars = calloc(length, sizeof(char));

    buffer.lines[buffer.numLines++] = (linebuf){
        .chars = chars,
        .render = chars,
        .cap = length,
        .length = 0,
    };

    // Insert at row index
    // error("bufferInsertLine() - Insert at not implemented");
}

// Free lines in buffer
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

    // Press ESC to exit

    while (1)
    {
        editorHandleInput();
    }

    editorExit();

    return EXIT_SUCCESS;
}