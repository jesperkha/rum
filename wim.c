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

#define cur_line (buffer.lines[buffer.cy])

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
    bufferCreateEmpty(4);
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

// Adds x, y to cursor position
void editorMoveCursor(int x, int y)
{
    buffer.cx += x;
    buffer.cy += y;

    if (buffer.cy < 0)
        buffer.cy = 0;
    
    if (buffer.cx < 0)
        buffer.cx = 0;

    if (buffer.cy > buffer.numLines - 1)
        buffer.cy = buffer.numLines - 1;
    
    if (buffer.cx > cur_line.length)
        buffer.cx = cur_line.length;

    editorUpdateCursorPos();
}

// Sets the cursor position to x, y
void editorSetCursorPos(int x, int y)
{
    COORD pos = {x, y};
    SetConsoleCursorPosition(editor.hstdout, pos);
}

void editorUpdateCursorPos()
{
    editorSetCursorPos(buffer.cx, buffer.cy);
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
                bufferInsertLine(buffer.cy + 1);
                break;
            
            case ARROW_UP:
                editorMoveCursor(0, -1);
                break;
            
            case ARROW_DOWN:
                editorMoveCursor(0, 1);
                break;

            case ARROW_LEFT:
                editorMoveCursor(-1, 0);
                break;

            case ARROW_RIGHT:
                editorMoveCursor(1, 0);
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

    editorSetCursorPos(0, 0);

    DWORD written;
    DWORD size = editor.width * editor.height;
    COORD origin = {0, 0};
    if (!FillConsoleOutputCharacter(editor.hstdout, (WCHAR)' ', size, origin, &written))
        return_error("editorClearScreen() - Failed to fill buffer");

    return RETURN_SUCCESS;
}

// ---------------------- BUFFER ----------------------

// Differs from editorSetCursorPos as it updates buffer.cx/cy.
// Also prints an error if coords are out of bounds.
void bufferSetCursorPos(int x, int y)
{
    if (y < 0 || y > buffer.numLines || x < 0 || x > buffer.lines[y].length)
    {
        error("bufferSetCursorPos() - Position out of bounds");
        return;
    }

    editorSetCursorPos(x, y);
    buffer.cx = x;
    buffer.cy = y;
}

// Writes text at given x, y. Does not move cursor.
void bufferWriteAt(int x, int y, const char* text)
{
    editorHideCursor();
    editorSetCursorPos(x, y);
    printf("%s", text);
    editorUpdateCursorPos();
    editorShowCursor();
}

// Renders the line found at given row index.
void bufferRenderLine(int row)
{
    editorHideCursor();
    linebuf line = buffer.lines[row];
    editorSetCursorPos(0, row);
    for (int i = 0; i < line.cap; i++)
        printf(" ");
    printf("| %d/%d ", line.length, line.cap);
    editorSetCursorPos(0, row);
    printf("%s", line.chars);
    editorUpdateCursorPos();
    editorShowCursor();
}

void bufferRenderLines(int offset)
{
    for (int i = offset; i < buffer.numLines; i++)
        bufferRenderLine(i);
}

// Write single character to current line.
void bufferWriteChar(char c)
{
    linebuf *line = &cur_line;
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
    bufferRenderLine(buffer.cy);
}

// Deletes the caharcter before the cursor position.
void bufferDeleteChar()
{
    linebuf *line = &cur_line;

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

    bufferRenderLine(buffer.cy);
}

// Inserts new line at row. If row is -1 line is appended to end of file.
void bufferInsertLine(int row)
{
    if (buffer.numLines == buffer.lineCap)
    {
        error("editorInsertNewLine() - Realloc line array in file buffer");
        return;
    }

    // Shift all lines down if not appended
    if (row < buffer.numLines)
    {
        size_t size = sizeof(linebuf) * (buffer.numLines - row);
        linebuf *pos = buffer.lines + row;
        memmove(pos + 1, pos, size);
    }

    int length = DEFUALT_LINE_LENGTH;
    char *chars = calloc(length, sizeof(char));

    buffer.lines[row] = (linebuf){
        .chars = chars,
        .render = chars,
        .cap = length,
        .length = 0,
    };

    buffer.numLines++;
    bufferRenderLines(row);
    bufferSetCursorPos(0, row);
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
    bufferInsertLine(0);
}

int main(void)
{
    editorInit();

    log("Press ESC to exit");

    while (1)
    {
        editorHandleInput();
    }

    editorExit();

    return EXIT_SUCCESS;
}