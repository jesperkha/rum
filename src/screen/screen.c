#include "rum.h"

extern Editor editor;
extern Config config;

void ScreenWrite(char *string, int length)
{
    TermWrite(string, length);
}

void ScreenWriteAt(int x, int y, char *text)
{
    CursorHide();
    CursorTempPos(x, y);
    ScreenWrite(text, strlen(text));
    CursorShow();
}

void ScreenColor(char *bg, char *fg)
{
    ScreenBg(bg);
    ScreenFg(fg);
}

void ScreenBg(char *bg)
{
    if (config.rawMode)
        return;
    char col[32];
    sprintf(col, "\x1b[48;2;%sm", bg);
    ScreenWrite(col, strlen(col));
}

void ScreenFg(char *fg)
{
    if (config.rawMode)
        return;
    char col[32];
    sprintf(col, "\x1b[38;2;%sm", fg);
    ScreenWrite(col, strlen(col));
}

#define COL_RESET "\x1b[0m"

void ScreenColorReset()
{
    if (config.rawMode)
        return;
    ScreenWrite(COL_RESET, 4);
}