#include <windows.h>
#include <stdio.h>

#include "wim.h"

#define error(msg) printf("Error: %s\n", msg);
#define log(msg) printf("Log: %s\n", msg);
#define error_return(msg) { error(msg); return RETURN_ERROR; }

struct editorGlobals
{
    HANDLE hstdin;  // Handle for standard input
    HANDLE hstdout; // Handle for standard output

    int width, height; // Size of terminal window
} editor;

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

    CONSOLE_SCREEN_BUFFER_INFO cinfo;
    if (!GetConsoleScreenBufferInfo(editor.hstdout, &cinfo))
    {
        error("editorInit() - Failed to get buffer info");
        exit(1);
    }

    editor.width = (int)cinfo.dwSize.X;
    editor.height = (int)cinfo.dwSize.Y;
}

// Returns the editor key code of the key pressed, -1 on error.
int readInputKey()
{
    char buffer[8];
    DWORD read;
    if (!ReadFile(editor.hstdin, buffer, 1, &read, NULL) || read == 0)
    {
        error("readInputKey() - Failed to read input");
        return RETURN_ERROR;
    }

    return buffer[0];
}

// Clears the terminal. Returns -1 on error.
int clearScreen()
{
    CONSOLE_SCREEN_BUFFER_INFO cinfo;
    if (!GetConsoleScreenBufferInfo(editor.hstdout, &cinfo))
        error_return("clearScreen() - Failed to get buffer info");
    
    COORD origin = {0, 0};
    SetConsoleCursorPosition(editor.hstdout, origin);

    DWORD written;
    DWORD size = editor.width * editor.height;
    if (!FillConsoleOutputCharacter(editor.hstdout, (WCHAR)' ', size, origin, &written))
        error_return("clearScreen() - Failed to fill buffer");
    
    return RETURN_SUCCESS;
}

int main(void)
{
    editorInit();
    SetConsoleMode(editor.hstdin, 0); // Set raw mode
    clearScreen();

    // Currently reads a single character from stdin and prints it out
    // Exits when q is pressed

    while (1)
    {
        int key = readInputKey();
        if (key == 'q')
            break;

        printf("%c", key);
    }

    return 0;
}