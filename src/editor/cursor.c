// Cursor manipulation, both in buffer space and terminal space.

#include "wim.h"

extern Editor editor;

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
    CursorSetPos(editor.col + x, editor.row + y, true);
}

// Sets cursor position in buffer space, scrolls if necessary. keepX is true when the cursor
// should keep the current max width when moving vertically, only really used with CursorMove.
void CursorSetPos(int x, int y, bool keepX)
{
    int dx = x - editor.col;
    int dy = y - editor.row;
    BufferScroll(dy); // Scroll by cursor offset

    editor.col = x;
    editor.row = y;

    if (editor.col < 0)
        editor.col = 0;
    if (editor.row < 0)
        editor.row = 0;
    if (editor.row > editor.numLines - 1)
        editor.row = editor.numLines - 1;
    if (editor.row - editor.offy > editor.textH)
        editor.row = editor.offy + editor.textH - editor.scrollDy;

    Line line = editor.lines[editor.row];

    // Keep cursor within bounds
    if (editor.col > line.length)
        editor.col = line.length;

    // Get indent for current line
    int i = 0;
    editor.indent = 0;
    while (i < editor.col && line.chars[i++] == ' ')
        editor.indent = i;

    // Keep cursor x when moving vertically
    if (keepX)
    {
        if (dy != 0)
        {
            if (editor.col > editor.colMax)
                editor.colMax = editor.col;
            if (editor.colMax <= line.length)
                editor.col = editor.colMax;
            if (editor.colMax > line.length)
                editor.col = line.length;
        }
        if (dx != 0)
            editor.colMax = editor.col;
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
    CursorSetPos(editor.col, editor.row, false);
}