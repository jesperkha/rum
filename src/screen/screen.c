#include "wim.h"

extern Editor editor;

void ScreenWrite(const char *string, int length)
{
    DWORD written;
    if (!WriteConsoleA(editor.hbuffer, string, length, &written, NULL) || written != length)
    {
        LogError("Failed to write to screen buffer");
        LogNumber("Written", written);
        LogNumber("Length", length);
        ExitProcess(1);
    }
}

void ScreenWriteAt(int x, int y, const char *text)
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
    char col[32];
    sprintf(col, "\x1b[48;2;%sm", bg);
    ScreenWrite(col, strlen(col));
}

void ScreenFg(char *fg)
{
    char col[32];
    sprintf(col, "\x1b[38;2;%sm", fg);
    ScreenWrite(col, strlen(col));
}

#define COL_RESET "\x1b[0m"

void ScreenColorReset()
{
    ScreenWrite(COL_RESET, 4);
}

void ScreenClearLine(int row)
{
    COORD pos = {0, row};
    DWORD written;
    FillConsoleOutputCharacterA(editor.hbuffer, ' ', editor.width, pos, &written);
}

void ScreenClear()
{
    DWORD written;
    COORD pos = {0, 0};
    int size = editor.width * editor.height;
    FillConsoleOutputCharacterA(editor.hbuffer, ' ', size, pos, &written);
}
