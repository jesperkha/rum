// This is the windows implementation for the generic OS interface include/os.h

#include "rum.h"

#ifdef OS_WINDOWS

#include <windows.h>
extern Editor editor;

void *MemAlloc(int size)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}

void *MemZeroAlloc(int size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

void *MemRealloc(void *ptr, int newSize)
{
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ptr, newSize);
}

void MemFree(void *ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}

char *OsReadFile(const char *filepath, int *size)
{
    // Open file. EditorOpenFile does not create files and fails on file-not-found
    HANDLE file = CreateFileA(filepath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        Error("failed to load file");
        return NULL;
    }

    // Get file size and read file contents into string buffer
    DWORD bufSize = GetFileSize(file, NULL) + 1;
    DWORD read;
    char *buffer = MemAlloc(bufSize);
    if (!ReadFile(file, buffer, bufSize, &read, NULL))
    {
        Error("failed to read file");
        CloseHandle(file);
        return NULL;
    }

    CloseHandle(file);
    *size = bufSize - 1;
    buffer[bufSize - 1] = 0;
    return buffer;
}

Status ReadTerminalInput(InputInfo *info)
{
    INPUT_RECORD record;
    DWORD read;
    if (!ReadConsoleInputA(GetStdHandle(STD_INPUT_HANDLE), &record, 1, &read) || read == 0)
        return RETURN_ERROR;

    info->eventType = INPUT_UNKNOWN;

    if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown)
    {
        KEY_EVENT_RECORD event = record.Event.KeyEvent;
        info->eventType = INPUT_KEYDOWN;
        info->keyCode = event.wVirtualKeyCode;
        info->asciiChar = event.uChar.AsciiChar;
        info->ctrlDown = event.dwControlKeyState & LEFT_CTRL_PRESSED;
    }
    else if (record.EventType == WINDOW_BUFFER_SIZE_EVENT)
        info->eventType = INPUT_WINDOW_RESIZE;

    return RETURN_SUCCESS;
}

// Creates and sets new terminal buffer
// Updates buffer size to terminal size
// Sets raw input mode
// Sets terminal title
// Disables cursor blinking
void TermInit(Editor *editor)
{
    // Create new temp buffer and set as active
    editor->hbuffer = CreateConsoleScreenBuffer(GENERIC_WRITE | GENERIC_READ, 0, NULL, 1, NULL);
    if (editor->hbuffer == INVALID_HANDLE_VALUE)
        Panic("failed to create new console screen buffer");

    SetConsoleActiveScreenBuffer(editor->hbuffer);
    TermUpdateSize(editor);

    COORD maxSize = GetLargestConsoleWindowSize(editor->hbuffer);
    if ((editor->renderBuffer = MemAlloc(maxSize.X * maxSize.Y * 4)) == NULL)
        Panic("failed to allocate renderBuffer");

    editor->hstdin = GetStdHandle(STD_INPUT_HANDLE);
    if (editor->hstdin == INVALID_HANDLE_VALUE)
        Panic("failed to get input handle");

    // 0 flag enabled 'raw' mode in terminal
    SetConsoleMode(editor->hstdin, 0);
    FlushConsoleInputBuffer(editor->hstdin);

    SetConsoleTitleA(TITLE);
    ScreenWrite("\033[?12l", 6); // Turn off cursor blinking
}

void TermUpdateSize(Editor *editor)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(editor->hbuffer, &info);

    short bufferW = info.dwSize.X;
    short windowH = info.srWindow.Bottom - info.srWindow.Top + 1;

    // Remove scrollbar by setting buffer height to window height
    COORD newSize;
    newSize.X = bufferW;
    newSize.Y = windowH;
    SetConsoleScreenBufferSize(editor->hbuffer, newSize);

    editor->width = (int)(newSize.X);
    editor->height = (int)(newSize.Y);
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
    if (!WriteConsoleA(editor.hbuffer, string, length, &written, NULL) || written != length)
    {
        Errorf("Failed to write to screen buffer. Length %d, written %d", length, (int)written);
        ExitProcess(1);
    }
}

#endif