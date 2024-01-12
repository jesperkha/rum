#include "wim.h"

extern Editor editor;

void ScreenWrite(const char *string, int length)
{
    DWORD written;
    if (!WriteConsoleA(editor.hbuffer, string, length, &written, NULL) || written != length)
    {
        LogError("Failed to write to screen buffer");
        EditorExit();
    }
}

void ScreenWriteAt(int x, int y, const char *text)
{
    CursorHide();
    CursorTempPos(x, y);
    ScreenWrite(text, strlen(text));
    CursorRestore();
    CursorShow();
}

void ScreenColor(int bg, int fg)
{
    ScreenBg(bg);
    ScreenFg(fg);
}

void ScreenBg(int bg)
{
    char col[32];
    sprintf(col, "\x1b[48;2;%sm", editor.colors + bg);
    ScreenWrite(col, strlen(col));
}

void ScreenFg(int fg)
{
    char col[32];
    sprintf(col, "\x1b[38;2;%sm", editor.colors + fg);
    ScreenWrite(col, strlen(col));
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
