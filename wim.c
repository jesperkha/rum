#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "wim.h"

#define log(msg)                  \
    editorWriteAt(0, 9, "Log: "); \
    editorWriteAt(5, 9, msg);
#define error(msg)                   \
    editorWriteAt(0, 10, "Error: "); \
    editorWriteAt(7, 10, msg);

// ---------------------- GLOBAL STATE ----------------------

typedef struct linebuf
{
    int idx; // Row index in file, not buffer
    int row; // Relative row in buffer, not file

    int cap;     // Capacity of line
    int length;  // Length of line
    char *chars; // Characters in line
} linebuf;

struct editorGlobals
{
    HANDLE hstdin;  // Handle for standard input
    HANDLE hstdout; // Handle for standard output
    DWORD mode_in;  // Restore to previous input mode

    int width, height; // Size of terminal window

    int row, col; // Current row and col of cursor in buffer
    int cx, cy;   // Relative cursor x, y (to buffer)

    int numLines, lineCap; // Count and capacity of lines in array
    linebuf *lines;        // Array of lines in buffer
} editor;

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

    bufferCreate();
    editorTerminalGetSize(); // Get new height of buffer
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

// Clears the terminal. Returns -1 on error.
int editorClearScreen()
{
    CONSOLE_SCREEN_BUFFER_INFO cinfo;
    if (!GetConsoleScreenBufferInfo(editor.hstdout, &cinfo))
        return_error("editorClearScreen() - Failed to get buffer info");

    cursorSetPos(0, 0);

    DWORD written;
    DWORD size = editor.width * editor.height;
    COORD origin = {0, 0};
    if (!FillConsoleOutputCharacter(editor.hstdout, (WCHAR)' ', size, origin, &written))
        return_error("editorClearScreen() - Failed to fill buffer");

    return RETURN_SUCCESS;
}

// Writes text at given x, y.
void editorWriteAt(int x, int y, const char *text)
{
    cursorHide();
    cursorTempPos(x, y);
    printf("%s", text);
    cursorRestore();
    cursorShow();
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

            case BACKSPACE:
                bufferDeleteChar();
                bufferRenderLine(editor.row);
                break;

            case ENTER:
                bufferInsertLine(editor.row + 1);
                bufferSplitLineDown(editor.row);
                cursorMove(0, 1);
                cursorSetPos(0, editor.cy);
                bufferRenderLines();
                break;

            case ARROW_UP:
                cursorMove(0, -1);
                break;

            case ARROW_DOWN:
                cursorMove(0, 1);
                break;

            case ARROW_LEFT:
                cursorMove(-1, 0);
                break;

            case ARROW_RIGHT:
                cursorMove(1, 0);
                break;

            default:
                bufferWriteChar(inputChar);
                bufferRenderLine(editor.row);
            }
        }
    }

    return RETURN_SUCCESS;
}

// ---------------------- CURSOR ----------------------

void cursorShow()
{
    CONSOLE_CURSOR_INFO info = {100, true};
    SetConsoleCursorInfo(editor.hstdout, &info);
}

void cursorHide()
{
    CONSOLE_CURSOR_INFO info = {100, false};
    SetConsoleCursorInfo(editor.hstdout, &info);
}

// Adds x, y to cursor position. Updates editor cursor position.
void cursorMove(int x, int y)
{
    if (
        // Cursor out of bounds
        (editor.cx <= 0 && x < 0) ||
        (editor.cy <= 0 && y < 0) ||
        (editor.cx >= editor.lines[editor.row].length && x > 0) ||
        (editor.cy >= editor.numLines - 1 && y > 0))
        return;

    editor.cx += x;
    editor.cy += y;
    editor.col += x;
    editor.row += y;

    int linelen = editor.lines[editor.row].length;
    if (editor.col > linelen)
    {
        editor.cx = linelen;
        editor.col = linelen;
    }

    cursorSetPos(editor.cx, editor.cy);
}

// Sets the cursor position to x, y. Updates editor cursor position.
void cursorSetPos(int x, int y)
{
    editor.cx = x;
    editor.cy = y;
    editor.col = x;
    editor.row = y;
    COORD pos = {x, y};
    SetConsoleCursorPosition(editor.hstdout, pos);
}

// Sets the cursor pos, does not update editor values. Restore with cursorRestore().
void cursorTempPos(int x, int y)
{
    COORD pos = {x, y};
    SetConsoleCursorPosition(editor.hstdout, pos);
}

// Restores cursor pos to where it was before call to cursorTempPos().
void cursorRestore()
{
    cursorSetPos(editor.cx, editor.cy);
}

// ---------------------- BUFFER ----------------------

// Creates an empty file buffer.
void bufferCreate()
{
    editor.numLines = 0;
    editor.lineCap = BUFFER_LINE_CAP;
    editor.lines = calloc(editor.lineCap, sizeof(linebuf));
    bufferCreateLine(0);
    bufferRenderLines();
}

// Free lines in buffer.
void bufferFree()
{
    for (int i = 0; i < editor.numLines; i++)
        free(editor.lines[i].chars);

    free(editor.lines);
}

// Write single character to current line.
void bufferWriteChar(char c)
{
    if (c < 32 || c > 126) // Reject non-ascii character
        return;

    linebuf *line = &editor.lines[editor.row];

    if (line->length >= line->cap - 1)
        // Extend line cap if exceeded
        bufferExtendLine(editor.row, line->cap + DEFAULT_LINE_LENGTH);

    if (editor.cx < line->length)
    {
        // Move text when typing in the middle of a line
        char *pos = line->chars + editor.col;
        memmove(pos + 1, pos, line->length - editor.col);
    }

    line->chars[editor.col] = c;
    line->length++;
    editor.cx++; // Todo: make line writechar function
    editor.col++;
}

// Deletes the caharcter before the cursor position.
void bufferDeleteChar()
{
    linebuf *line = &editor.lines[editor.row];

    if (editor.col == 0)
    {
        if (editor.row == 0)
            return;

        // Delete line if cursor is at start
        int cur_row = editor.row; // Cursor setpos modifies editor.row
        cursorSetPos(editor.lines[editor.row - 1].length, editor.cy - 1);
        bufferSplitLineUp(cur_row);
        bufferDeleteLine(cur_row);
        bufferRenderLines();
        return;
    }

    if (editor.col <= line->length)
    {
        // Move chars when deleting in middle of line
        char *pos = line->chars + editor.col;
        memmove(pos - 1, pos, line->length - editor.col);
        line->chars[--line->length] = 0;
    }
    else
        line->chars[--line->length] = 0;

    editor.cx--;
    editor.col--;
}

// Renders the line found at given row index.
void bufferRenderLine(int row)
{
    // Todo: better line render
    linebuf line = editor.lines[row];
    cursorHide();
    cursorTempPos(0, row);
    for (int i = 0; i < line.cap; i++)
        printf(" ");
    printf(" | %d", line.cap);
    cursorTempPos(0, row);
    printf("%s", line.chars);
    cursorRestore();
    cursorShow();
}

// Renders all visible lines in buffer
void bufferRenderLines()
{
    for (int i = 0; i < editor.numLines; i++)
        bufferRenderLine(i);
}

// Creates an empty line at idx. Does not resize array.
void bufferCreateLine(int idx)
{
    linebuf line = {
        .chars = calloc(DEFAULT_LINE_LENGTH, sizeof(char)),
        .cap = DEFAULT_LINE_LENGTH,
        .row = idx,
        .length = 0,
        .idx = 0,
    };

    memcpy(&editor.lines[idx], &line, sizeof(linebuf));
    editor.numLines++;
}

// Realloc line character buffer
void bufferExtendLine(int row, int new_size)
{
    linebuf *line = &editor.lines[editor.row];
    line->cap = new_size;
    line->chars = realloc(line->chars, line->cap);
    memset(line->chars + line->length, 0, line->cap - line->length);
}

// Inserts new line at row. If row is -1 line is appended to end of file.
void bufferInsertLine(int row)
{
    row = row != -1 ? row : editor.numLines;

    if (editor.numLines >= editor.lineCap)
    {
        // Realloc editor line buffer array when full
        size_t new_size = editor.lineCap + BUFFER_LINE_CAP;
        editor.lines = realloc(editor.lines, new_size * sizeof(linebuf));
        editor.lineCap = new_size;
    }

    if (row < editor.numLines)
    {
        // Move lines down when adding newline mid-file
        linebuf *pos = editor.lines + row;
        size_t count = editor.numLines - row;
        memmove(pos + 1, pos, count * sizeof(linebuf));
    }

    bufferCreateLine(row);
}

// Removes line at row.
void bufferDeleteLine(int row)
{
    free(editor.lines[row].chars);
    linebuf *pos = editor.lines + row + 1;
    size_t count = editor.numLines - row;
    memmove(pos - 1, pos, count * sizeof(linebuf));
    memset(editor.lines + editor.numLines, 0, sizeof(linebuf));
    editor.numLines--;
}

// Moves characters behind cursor down to line below.
void bufferSplitLineDown(int row)
{
    linebuf *from = &editor.lines[row];
    linebuf *to = &editor.lines[row + 1];
    size_t length = from->length - editor.col;

    if (to->cap < length)
    {
        // Realloc line buffer so new text fits
        int l = DEFAULT_LINE_LENGTH;
        bufferExtendLine(row + 1, (length / l) * l + l);
    }

    // Copy characters and set right side of row to 0
    strcpy(to->chars, from->chars + editor.col);
    memset(from->chars + editor.col, 0, length);
    to->length = length;
    from->length -= length;
}

// Moves characters behind cursor to end of line above and deletes line.
void bufferSplitLineUp(int row)
{
    linebuf *from = &editor.lines[row];
    linebuf *to = &editor.lines[row - 1];

    if (from->length == 0)
        return;

    size_t length = from->length - editor.col + to->length;
    if (to->cap < length)
    {
        // Realloc line buffer so new text fits
        int l = DEFAULT_LINE_LENGTH;
        bufferExtendLine(row - 1, (length / l) * l + l);
    }

    memcpy(to->chars + to->length, from->chars, from->length);
    to->length += from->length;
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