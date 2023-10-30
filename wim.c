#include <windows.h>
#include <stdio.h>

#include "wim.h"

#define error(msg) printf("Error: %s\n", msg);
#define log(msg) printf("Log: %s\n", msg);
#define return_error(msg)    \
    {                        \
        error(msg);          \
        return RETURN_ERROR; \
    }

struct editorGlobals
{
    HANDLE hstdin;  // Handle for standard input
    HANDLE hstdout; // Handle for standard output

    int width, height; // Size of terminal window
} editor;

int terminalResize();

// Populates editor global struct. Exits on error.
void editorInit()
{
    editor.hstdin = GetStdHandle(STD_INPUT_HANDLE);
    editor.hstdout = GetStdHandle(STD_OUTPUT_HANDLE);

    if (editor.hstdin == NULL || editor.hstdout == NULL)
    {
        error("editorInit() - Failed to get std handles");
        exit(1);
    }

    if (terminalResize() == RETURN_ERROR)
    {
        error("editorInit() - Failed to get window size");
        exit(1);
    }
}

// Update editor size values. Returns -1 on error.
int terminalResize()
{
    CONSOLE_SCREEN_BUFFER_INFO cinfo;
    if (!GetConsoleScreenBufferInfo(editor.hstdout, &cinfo))
        return_error("windowResize() - Failed to get buffer info");

    editor.width = (int)cinfo.srWindow.Right;
    editor.height = (int)(cinfo.srWindow.Bottom + 1);

    return RETURN_SUCCESS;
}

// Returns the editor key code of the key pressed, -1 on error.
int readInputKey()
{
    char buffer[8];
    DWORD read;
    if (!ReadFile(editor.hstdin, buffer, 1, &read, NULL) || read == 0)
        return_error("readInputKey() - Failed to read input");

    return buffer[0];
}

// Clears the terminal. Returns -1 on error.
int clearScreen()
{
    CONSOLE_SCREEN_BUFFER_INFO cinfo;
    if (!GetConsoleScreenBufferInfo(editor.hstdout, &cinfo))
        return_error("clearScreen() - Failed to get buffer info");

    COORD origin = {0, 0};
    SetConsoleCursorPosition(editor.hstdout, origin);

    DWORD written;
    DWORD size = editor.width * editor.height;
    if (!FillConsoleOutputCharacter(editor.hstdout, (WCHAR)' ', size, origin, &written))
        return_error("clearScreen() - Failed to fill buffer");

    return RETURN_SUCCESS;
}

int main(void)
{
    editorInit();
    clearScreen();
    // SetConsoleMode(editor.hstdin, 0); // Set raw mode

    terminalResize(); // Get new size of buffer after clear

    return 0;
}