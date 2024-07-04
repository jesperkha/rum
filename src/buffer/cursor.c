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

void CursorMove(Buffer *b, int x, int y)
{
    CursorSetPos(b, b->cursor.col + x, b->cursor.row + y, true);
}

// Sets cursor position in buffer space, scrolls if necessary. keepX is true when the cursor
// should keep the current max width when moving vertically, only really used with CursorMove.
void CursorSetPos(Buffer *b, int x, int y, bool keepX)
{
    Cursor *c = &b->cursor;

    int dx = x - c->col;
    int dy = y - c->row;
    BufferScroll(b, dy); // Scroll by cursor offset

    c->col = x;
    c->row = y;

    if (c->row < 0)
        c->row = 0;
    if (c->col < 0)
        c->col = 0;
    if (c->row > b->numLines - 1)
        c->row = b->numLines - 1;

    Line *line = &b->lines[c->row];

    if (c->col > line->length)
        c->col = line->length;

    // Get indent for current line
    int i = 0;
    c->indent = 0;
    while (i < line->length && line->chars[i++] == ' ')
        c->indent = i;
    line->indent = c->indent;

    // Keep cursor x when moving vertically
    if (dy != 0)
    {
        if (c->col > c->colMax)
            c->colMax = c->col;
        if (c->colMax <= line->length && keepX)
            c->col = c->colMax;
        if (c->colMax > line->length && keepX)
            c->col = line->length;
    }
    if (dx != 0)
        c->colMax = c->col;
}

// Sets the cursor pos without additional stuff happening. The editor position is
// not updated so cursor returns to previous position when render is called.
void CursorTempPos(int x, int y)
{
    COORD pos = {x, y};
    SetConsoleCursorPosition(editor.hbuffer, pos);
}
