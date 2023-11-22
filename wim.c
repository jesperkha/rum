#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>

#include "wim.h"

// Debug
void logError(const char *msg);
#define check_pointer(ptr, where)       \
    if (ptr == NULL)                    \
    {                                   \
        logError("NULL pointer alloc"); \
        logError(where);                \
        exit(1);                        \
    }

// Debug
void logNumber(const char *what, int number)
{
    FILE *f = fopen("log", "a");
    check_pointer(f, "logToFile");
    fprintf(f, "[ LOG ]: %s, %d\n", what, number);
    fclose(f);
}

// Debug
void logError(const char *msg)
{
    FILE *f = fopen("log", "a");
    check_pointer(f, "logError");
    fprintf(f, "[ ERROR ]: %s, Windows error code: %d\n", msg, (int)GetLastError());
    fclose(f);
}

// Debug (use stderr and quit after)
#define return_error(msg)    \
    {                        \
        logError(msg);       \
        return RETURN_ERROR; \
    }

// Debug
void *__calloc(size_t count, size_t size)
{
    logNumber("Calloc", count * size);
    void *mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, count * size);
    if (mem == NULL)
    {
        logError("Calloc failed");
        editorExit();
    }

    return mem;
}

// Debug
void *__realloc(void *ptr, size_t newsize)
{
    logNumber("Realloc", newsize);
    void *mem = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ptr, newsize);
    if (mem == NULL)
    {
        logError("Realloc failed");
        editorExit();
    }

    return mem;
}

// Debug
void __free(void *ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}

// Debug
#define calloc __calloc
#define realloc __realloc
#define free __free

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
    HANDLE hbuffer; // Handle to new screen buffer

    int width, height; // Size of terminal window
    int row, col;      // Current row and col of cursor in buffer
    int offx, offy;    // x, y offset from left/top

    int numLines, lineCap; // Count and capacity of lines in array
    linebuf *lines;        // Array of lines in buffer
    char *renderBuffer;    // Written to and printed on render
} editor;

// ---------------------- EDITOR ----------------------

// Populates editor global struct and creates empty file buffer. Exits on error.
void editorInit()
{
    editor.hstdin = GetStdHandle(STD_INPUT_HANDLE);
    editor.hbuffer = CreateConsoleScreenBuffer(
        GENERIC_WRITE | GENERIC_READ, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);

    if (editor.hbuffer == INVALID_HANDLE_VALUE || editor.hstdin == INVALID_HANDLE_VALUE)
    {
        logError("editorInit() - Failed to get one or more handles");
        ExitProcess(EXIT_FAILURE);
    }

    SetConsoleActiveScreenBuffer(editor.hbuffer); // Swap buffer
    SetConsoleMode(editor.hstdin, 0);             // Set raw input mode
    SetConsoleTitle(TITLE);
    FlushConsoleInputBuffer(editor.hstdin);

    if (editorTerminalGetSize() == RETURN_ERROR)
    {
        logError("editorInit() - Failed to get buffer size");
        ExitProcess(EXIT_FAILURE);
    }

    editor.offx = 3;
    editor.offy = 0;

    screenBufferClearAll();
    bufferCreate();
}

// Free, clean, and exit
void editorExit()
{
    bufferFree();
    CloseHandle(editor.hbuffer);
    ExitProcess(EXIT_SUCCESS);
}

// Update editor size values. Returns -1 on error.
int editorTerminalGetSize()
{
    CONSOLE_SCREEN_BUFFER_INFO cinfo;
    if (!GetConsoleScreenBufferInfo(editor.hbuffer, &cinfo))
        return_error("editorTerminalGetSize() - Failed to get buffer info");

    editor.width = (int)cinfo.srWindow.Right;
    editor.height = (int)(cinfo.srWindow.Bottom + 1);

    return RETURN_SUCCESS;
}

// Writes text at given x, y.
void editorWriteAt(int x, int y, const char *text)
{
    cursorHide();
    cursorTempPos(x, y);
    screenBufferWrite(text, strlen(text));
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
            case K_ESCAPE:
                editorExit();

            case K_BACKSPACE:
                bufferDeleteChar();
                renderLine(editor.row);
                break;

            // Todo: wtf is this shit
            case K_DELETE:
            {
                if (editor.col == editor.lines[editor.row].length)
                {
                    if (editor.row == editor.numLines - 1)
                        break;

                    cursorHide();
                    cursorSetPos(0, editor.row + 1);
                }
                else
                {
                    cursorHide();
                    cursorMove(1, 0);
                }

                bufferDeleteChar();
                cursorShow();
                renderLines();
                break;
            }

            case K_ENTER:
                bufferInsertLine(editor.row + 1);
                bufferSplitLineDown(editor.row);
                cursorSetPos(0, editor.row + 1);
                renderLines();
                break;

            case K_ARROW_UP:
                cursorMove(0, -1);
                break;

            case K_ARROW_DOWN:
                cursorMove(0, 1);
                break;

            case K_ARROW_LEFT:
                cursorMove(-1, 0);
                break;

            case K_ARROW_RIGHT:
                cursorMove(1, 0);
                break;

            default:
                bufferWriteChar(inputChar);
                renderLine(editor.row);
            }
        }
    }

    return RETURN_SUCCESS;
}

// ---------------------- SCREEN BUFFER ----------------------

// Write line to screen buffer
void screenBufferWrite(const char *string, int length)
{
    DWORD written;
    if (!WriteConsoleA(editor.hbuffer, string, length, &written, NULL) || written != length)
    {
        logError("Failed to write to screen buffer");
        editorExit();
    }
}

// Fills line with blanks
void screenBufferClearLine(int row)
{
    COORD pos = {0, row};
    DWORD written;
    FillConsoleOutputCharacter(editor.hbuffer, (WCHAR)' ', editor.width, pos, &written);
}

// Clears the whole buffer
void screenBufferClearAll()
{
    DWORD written;
    COORD pos = {0, 0};
    int size = editor.width * editor.height;
    FillConsoleOutputCharacter(editor.hbuffer, (WCHAR)' ', size, pos, &written);
}

// ---------------------- CURSOR ----------------------

void cursorShow()
{
    CONSOLE_CURSOR_INFO info = {100, true};
    SetConsoleCursorInfo(editor.hbuffer, &info);
}

void cursorHide()
{
    CONSOLE_CURSOR_INFO info = {100, false};
    SetConsoleCursorInfo(editor.hbuffer, &info);
}

// Adds x, y to cursor position. Updates editor cursor position.
void cursorMove(int x, int y)
{
    // Todo: implement vertical scroll

    if (
        // Cursor out of bounds
        (editor.col <= 0 && x < 0) ||
        (editor.row <= 0 && y < 0) ||
        (editor.col >= editor.lines[editor.row].length && x > 0) ||
        (editor.row >= editor.numLines - 1 && y > 0))
        return;

    editor.col += x;
    editor.row += y;

    int linelen = editor.lines[editor.row].length;
    if (editor.col > linelen)
        editor.col = linelen;

    cursorSetPos(editor.col, editor.row);
}

// Sets the cursor position to x, y. Updates editor cursor position.
void cursorSetPos(int x, int y)
{
    editor.col = x;
    editor.row = y;
    COORD pos = {x + editor.offx, y + editor.offy};
    SetConsoleCursorPosition(editor.hbuffer, pos);
}

// Sets the cursor pos, does not update editor values. Restore with cursorRestore().
void cursorTempPos(int x, int y)
{
    COORD pos = {x, y};
    SetConsoleCursorPosition(editor.hbuffer, pos);
}

// Restores cursor pos to where it was before call to cursorTempPos().
void cursorRestore()
{
    cursorSetPos(editor.col, editor.row);
}

// ---------------------- BUFFER ----------------------

// Creates an empty file buffer.
void bufferCreate()
{
    editor.numLines = 0;
    editor.lineCap = BUFFER_LINE_CAP;
    editor.lines = calloc(editor.lineCap, sizeof(linebuf));
    editor.renderBuffer = malloc(editor.width * editor.height);
    check_pointer(editor.lines, "bufferCreate");
    check_pointer(editor.renderBuffer, "bufferCreate");
    bufferCreateLine(0);
    renderLines();
}

// Free lines in buffer.
void bufferFree()
{
    for (int i = 0; i < editor.numLines; i++)
        free(editor.lines[i].chars);

    free(editor.lines);
    free(editor.renderBuffer);
}

// Write single character to current line.
void bufferWriteChar(char c)
{
    if (c < 32 || c > 126) // Reject non-ascii character
        return;

    if (editor.col >= editor.width - 1)
        // Todo: implement horizontal text scrolling
        return;

    linebuf *line = &editor.lines[editor.row];

    if (line->length >= line->cap - 1)
        // Extend line cap if exceeded
        bufferExtendLine(editor.row, line->cap + DEFAULT_LINE_LENGTH);

    if (editor.col < line->length)
    {
        // Move text when typing in the middle of a line
        char *pos = line->chars + editor.col;
        memmove(pos + 1, pos, line->length - editor.col);
    }

    line->chars[editor.col] = c;
    line->length++;
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
        cursorSetPos(editor.lines[editor.row - 1].length, editor.row - 1);
        bufferSplitLineUp(cur_row);
        bufferDeleteLine(cur_row);
        renderLines();
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

    editor.col--;
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

    check_pointer(line.chars, "bufferCreateLine");
    memcpy(&editor.lines[idx], &line, sizeof(linebuf));
    editor.numLines++;
}

// Realloc line character buffer
void bufferExtendLine(int row, int new_size)
{
    linebuf *line = &editor.lines[row];
    line->cap = new_size;
    line->chars = realloc(line->chars, line->cap);
    check_pointer(line->chars, "bufferExtendLine");
    memset(line->chars + line->length, 0, line->cap - line->length);
}

// Inserts new line at row. If row is -1 line is appended to end of file.
void bufferInsertLine(int row)
{
    row = row != -1 ? row : editor.numLines;

    if (editor.numLines >= editor.lineCap)
    {
        // Realloc editor line buffer array when full
        editor.lineCap += BUFFER_LINE_CAP;
        editor.lines = realloc(editor.lines, editor.lineCap * sizeof(linebuf));
        check_pointer(editor.lines, "bufferInsertLine");
    }

    if (row < editor.numLines)
    {
        // Move lines down when adding newline mid-file
        linebuf *pos = editor.lines + row;
        int count = editor.numLines - row;
        memmove(pos + 1, pos, count * sizeof(linebuf));
    }

    bufferCreateLine(row);
}

// Removes line at row.
void bufferDeleteLine(int row)
{
    free(editor.lines[row].chars);
    linebuf *pos = editor.lines + row + 1;

    if (row != editor.lineCap - 1)
    {
        int count = editor.numLines - row;
        memmove(pos - 1, pos, count * sizeof(linebuf));
        memset(editor.lines + editor.numLines, 0, sizeof(linebuf));
    }

    editor.numLines--;
}

// Moves characters behind cursor down to line below.
void bufferSplitLineDown(int row)
{
    if (row == editor.lineCap - 1)
    {
        // Debug
        logError("Failed to split down, not newline");
        return;
    }

    linebuf *from = &editor.lines[row];
    linebuf *to = &editor.lines[row + 1];
    int length = from->length - editor.col;

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
    if (row == 0)
    {
        // Debug
        logError("Failed to split up, first row");
        return;
    }

    linebuf *from = &editor.lines[row];
    linebuf *to = &editor.lines[row - 1];

    if (from->length == 0)
        return;

    int length = from->length - editor.col + to->length;
    if (to->cap < length)
    {
        // Realloc line buffer so new text fits
        int l = DEFAULT_LINE_LENGTH;
        bufferExtendLine(row - 1, (length / l) * l + l);
    }

    memcpy(to->chars + to->length, from->chars, from->length);
    to->length += from->length;
}

// ---------------------- RENDER ----------------------

// Renders the line found at given row index.
void renderLine(int row)
{
    linebuf line = editor.lines[row];
    cursorHide();
    cursorTempPos(0, row);
    screenBufferClearLine(row);
    screenBufferWrite(":: ", 3);
    screenBufferWrite(line.chars, line.length);
    cursorRestore();
    cursorShow();
}

// Renders all visible lines in buffer
void renderLines()
{
    screenBufferClearAll();
    for (int i = 0; i < editor.numLines; i++)
        renderLine(i);
}

// Does everything
void renderBuffer()
{
    cursorHide();
    cursorShow();
}

int main(void)
{
    // Debug: clear log file
    FILE *f = fopen("log", "w");
    fclose(f);

    editorInit();

    while (1)
    {
        editorHandleInput();
    }

    editorExit();

    return EXIT_SUCCESS;
}