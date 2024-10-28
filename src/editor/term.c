#include "rum.h"

extern Editor editor;

void TermUpdateSize()
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(editor.hbuffer, &info);

    short bufferW = info.dwSize.X;
    short windowH = info.srWindow.Bottom - info.srWindow.Top + 1;

    // Remove scrollbar by setting buffer height to window height
    COORD newSize;
    newSize.X = bufferW;
    newSize.Y = windowH;
    SetConsoleScreenBufferSize(editor.hbuffer, newSize);

    editor.width = (int)(newSize.X);
    editor.height = (int)(newSize.Y);
}

void TermSetCursorPos(int x, int y)
{
    COORD pos = {x, y};
    SetConsoleCursorPosition(editor.hbuffer, pos);
}

void TermSetCursorVisible(bool visible)
{
    CONSOLE_CURSOR_INFO info = {100, visible};
    SetConsoleCursorInfo(editor.hbuffer, &info);
}

void TermWrite(char *string, int length)
{
    DWORD written;
    if (!WriteConsoleA(editor.hbuffer, string, length, &written, NULL) || (int)written != length)
        Panicf("Failed to write to screen buffer. Length %d, written %d", length, (int)written);
}