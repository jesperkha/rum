// Cursor manipulation, both in buffer space and terminal space.

#include "rum.h"

extern Editor editor;

void CursorShow()
{
    TermSetCursorVisible(true);
}

void CursorHide()
{
    TermSetCursorVisible(false);
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

    c->col = x;
    c->row = y;

    if (c->row < 0)
        c->row = 0;
    if (c->col < 0)
        c->col = 0;
    if (c->row > b->numLines - 1)
        c->row = b->numLines - 1;

    Line *line = &b->lines[c->row];
    int maxCol = line->length;
    capValue(c->col, maxCol);

    // Get indent for current line
    c->indent = 0;
    for (int i = 0; i < line->length; i++)
    {
        if (line->chars[i] != ' ')
            break;
        c->indent++;
    }
    line->indent = c->indent;

    // Keep cursor x when moving vertically
    if (dy != 0)
    {
        if (c->col > c->colMax)
            c->colMax = c->col;
        if (c->colMax <= maxCol && keepX)
            c->col = c->colMax;
        if (c->colMax > maxCol && keepX)
            c->col = maxCol;
    }
    if (dx != 0)
        c->colMax = c->col;

    BufferScroll(b);
}

// Sets the cursor pos without additional stuff happening. The editor position is
// not updated so cursor returns to previous position when render is called.
void CursorTempPos(int x, int y)
{
    TermSetCursorPos(x, y);
}

void CursorUpdatePos()
{
    int x = curBuffer->cursor.col - curBuffer->cursor.offx + curBuffer->padX + curBuffer->offX;
    int y = curBuffer->cursor.row - curBuffer->cursor.offy + curBuffer->padY;
    TermSetCursorPos(x, y);
}