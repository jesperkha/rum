#include "wim.h"

extern Editor editor;

// Writes at cursor position
void screenBufferWrite(const char *string, int length)
{
    DWORD written;
    if (!WriteConsoleA(editor.hbuffer, string, length, &written, NULL) || written != length)
    {
        LogError("Failed to write to screen buffer");
        EditorExit();
    }
}

void screenBufferBg(int col)
{
    screenBufferWrite("\x1b[48;2;", 7);
    screenBufferWrite(editor.colors+col, 11);
    screenBufferWrite("m", 1);
}

void screenBufferFg(int col)
{
    screenBufferWrite("\x1b[38;2;", 7);
    screenBufferWrite(editor.colors+col, 11);
    screenBufferWrite("m", 1);
}

void screenBufferClearLine(int row)
{
    COORD pos = {0, row};
    DWORD written;
    FillConsoleOutputCharacterA(editor.hbuffer, ' ', editor.width, pos, &written);
}

void screenBufferClearAll()
{
    DWORD written;
    COORD pos = {0, 0};
    int size = editor.width * editor.height;
    FillConsoleOutputCharacterA(editor.hbuffer, ' ', size, pos, &written);
}