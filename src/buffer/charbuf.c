// Helper struct for accumulating strings to print.

#include "wim.h"

extern Editor editor;

// Returns pointer to empty CharBuf mapped to input buffer.
CharBuf *CbNew(char *buffer)
{
    CharBuf *b = memAlloc(sizeof(CharBuf));
    b->buffer = buffer;
    b->pos = buffer;
    b->lineLength = 0;
    return b;
}

// Resets buffer to starting state. Does not memclear the internal buffer.
void CbReset(CharBuf *buf)
{
    buf->pos = buf->buffer;
    buf->lineLength = 0;
}

void CbAppend(CharBuf *buf, char *src, int length)
{
    memcpy(buf->pos, src, length);
    buf->pos += length;
    buf->lineLength += length;
}

// Fills remaining line with space characters based on editor width.
void CbNextLine(CharBuf *buf)
{
    int size = editor.width - buf->lineLength;
    for (int i = 0; i < size; i++)
        *(buf->pos++) = ' ';
    buf->lineLength = 0;
}

// Adds background and foreground color to buffer.
void CbColor(CharBuf *buf, int bg, int fg)
{
    CbBg(buf, bg);
    CbFg(buf, fg);
}

void CbBg(CharBuf *buf, int bg)
{
    char *c = editor.colors;
    char col[32];
    sprintf(col, "\x1b[48;2;%sm", c + bg);
    int length = strlen(col);
    memcpy(buf->pos, col, length);
    buf->pos += length;
}

void CbFg(CharBuf *buf, int fg)
{
    char *c = editor.colors;
    char col[32];
    sprintf(col, "\x1b[38;2;%sm", c + fg);
    int length = strlen(col);
    memcpy(buf->pos, col, length);
    buf->pos += length;
}

// Adds COL_RESET to buffer
void CbColorReset(CharBuf *buf)
{
    int length = strlen(COL_RESET);
    memcpy(buf->pos, COL_RESET, length);
    buf->pos += length;
}

// Prints buffer at x, y with accumulated length only.
void CbRender(CharBuf *buf, int x, int y)
{
    cursorHide();
    cursorTempPos(x, y);
    screenBufferWrite(buf->buffer, buf->pos - buf->buffer);
    cursorRestore();
    cursorShow();
}
