// Helper struct for accumulating strings to print.

#include "rum.h"

extern Editor editor;
extern Config config;

// Returns empty CharBuf mapped to input buffer.
CharBuf CbNew(char *buffer)
{
    CharBuf b;
    b.buffer = buffer;
    b.pos = buffer;
    b.lineLength = 0;
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

void CbRepeat(CharBuf *buf, char c, int count)
{
    for (int i = 0; i < count; i++)
        *(buf->pos++) = c;
    buf->lineLength += count;
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
void CbColor(CharBuf *buf, char *bg, char *fg)
{
    CbBg(buf, bg);
    CbFg(buf, fg);
}

void CbColorWord(CharBuf *cb, char *fg, char *word, int wordlen)
{
    CbFg(cb, fg);
    CbAppend(cb, word, wordlen);
}

void CbBg(CharBuf *buf, char *bg)
{
    if (config.rawMode)
        return;
    char col[32];
    sprintf(col, "\x1b[48;2;%sm", bg);
    int length = strlen(col);
    memcpy(buf->pos, col, length);
    buf->pos += length;
}

void CbFg(CharBuf *buf, char *fg)
{
    if (config.rawMode)
        return;
    char col[32];
    sprintf(col, "\x1b[38;2;%sm", fg);
    int length = strlen(col);
    memcpy(buf->pos, col, length);
    buf->pos += length;
}

// Resets colors in buffer
void CbColorReset(CharBuf *buf)
{
    if (config.rawMode)
        return;
    int length = strlen(COL_RESET);
    memcpy(buf->pos, COL_RESET, length);
    buf->pos += length;
}

// Prints buffer at x, y with accumulated length only.
void CbRender(CharBuf *buf, int x, int y)
{
    CursorHide();
    CursorTempPos(x, y);
    ScreenWrite(buf->buffer, buf->pos - buf->buffer);
    CursorShow();
}

int CbLength(CharBuf *cb)
{
    return cb->pos - cb->buffer;
}