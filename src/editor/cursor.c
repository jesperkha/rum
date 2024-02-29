// Cursor manipulation, both in buffer space and terminal space.

#include "wim.h"

extern Editor editor;
extern Buffer buffer;

void CursorShow()
{
    CONSOLE_CURSOR_INFO info = {100, true};
    SetConsoleCursorInfo(editor.hbuffer, &info);
}

void CursorHide()
{
    CONSOLE_CURSOR_INFO info = {100, false};
    SetConsoleCursorInfo(editor.hbuffer, &info);
}

void CursorMove(int x, int y)
{
    CursorSetPos(buffer.cursor.col + x, buffer.cursor.row + y, true);
}

// Sets cursor position in buffer space, scrolls if necessary. keepX is true when the cursor
// should keep the current max width when moving vertically, only really used with CursorMove.
void CursorSetPos(int x, int y, bool keepX)
{
    int dx = x - buffer.cursor.col;
    int dy = y - buffer.cursor.row;
    BufferScroll(&buffer, dy); // Scroll by cursor offset

    buffer.cursor.col = x;
    buffer.cursor.row = y;

    if (buffer.cursor.col < 0)
        buffer.cursor.col = 0;
    if (buffer.cursor.row < 0)
        buffer.cursor.row = 0;
    if (buffer.cursor.row > buffer.numLines - 1)
        buffer.cursor.row = buffer.numLines - 1;
    // if (buffer.cursor.row - buffer.cursor.offy > buffer.textH)
    //     buffer.cursor.row = buffer.cursor.offy + buffer.textH - buffer.cursor.scrollDy;

    Line line = buffer.lines[buffer.cursor.row];

    // Keep cursor within bounds
    if (buffer.cursor.col > line.length)
        buffer.cursor.col = line.length;

    // Get indent for current line
    int i = 0;
    buffer.cursor.indent = 0;
    while (i < buffer.cursor.col && line.chars[i++] == ' ')
        buffer.cursor.indent = i;

    // Keep cursor x when moving vertically
    if (keepX)
    {
        if (dy != 0)
        {
            if (buffer.cursor.col > buffer.cursor.colMax)
                buffer.cursor.colMax = buffer.cursor.col;
            if (buffer.cursor.colMax <= line.length)
                buffer.cursor.col = buffer.cursor.colMax;
            if (buffer.cursor.colMax > line.length)
                buffer.cursor.col = line.length;
        }
        if (dx != 0)
            buffer.cursor.colMax = buffer.cursor.col;
    }
}

// Sets the cursor pos without additional stuff happening. The editor position is
// not updated so cursor returns to previous position when render is called.
void CursorTempPos(int x, int y)
{
    COORD pos = {x, y};
    SetConsoleCursorPosition(editor.hbuffer, pos);
}

// Restores cursor position to editor pos.
void CursorRestore()
{
    CursorSetPos(buffer.cursor.col, buffer.cursor.row, false);
}