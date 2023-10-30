#include <windows.h>
#include <stdio.h>
#include <stdbool.h>

#include "wim.h"

struct editorGlobals
{
    HANDLE hstdin;  // Handle for standard input
    HANDLE hstdout; // Handle for standard output

    int width, height; // Size of terminal window
} editor;

struct editorFileBuffer
{
    int cx, cy;            // Cursor x, y
    int numLines, lineCap; // Size and capacity of line array
    linebuf *lines;        // Array of line buffers
} buffer;

// ---------------------- EDITOR ----------------------

// Populates editor global struct and creates empty file buffer. Exits on error.
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

    bufferCreateEmpty(16);
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

// Takes action based on current user input. Returns -1 on error.
int handleUserInput()
{
    char inputBuffer[8];
    DWORD read;
    if (!ReadFile(editor.hstdin, inputBuffer, 2, &read, NULL) || read == 0)
        return_error("readUserInput() - Failed to read input");

    // Assume normal character input
    char input = inputBuffer[0];
    bufferWriteChar(&buffer.lines[buffer.cy], input);

    return RETURN_SUCCESS;
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

// ---------------------- BUFFER ----------------------

void bufferRenderLine(linebuf *line)
{
    // Todo: render line properly
    printf("\r%s", line->chars);
}

// Write single character to current line. Returns -1 on error.
int bufferWriteChar(linebuf *line, char c)
{
    if (c == 'q')
        exit(0);

    line->chars[buffer.cx++] = c;
    bufferRenderLine(line);
    return RETURN_SUCCESS;
}

// Inserts new line at row. If row is -1 line is appended to end of file.
void bufferInsertLine(int row)
{
    // Append to end of file
    if (row == -1)
    {
        if (buffer.lineCap == buffer.numLines + 1)
        {
            error("editorInsertNewLine() - Realloc line array in file buffer");
            return;
        }

        int length = DEFUALT_LINE_LENGTH;
        char *chars = calloc(length, sizeof(char));

        buffer.lines[buffer.numLines++] = (linebuf){
            .chars = chars,
            .render = chars,
            .cap = length,
        };

        return;
    }

    // Insert at row index
}

void bufferFree()
{
    for (int i = 0; i < buffer.numLines; i++)
        free(buffer.lines[i].chars);
}

// Creates an empty file buffer with line cap n.
void bufferCreateEmpty(int n)
{
    buffer.lineCap = n;
    buffer.numLines = 0;
    buffer.lines = calloc(n, sizeof(linebuf));
    bufferInsertLine(-1);
}

int main(void)
{
    editorInit();
    clearScreen();
    terminalResize(); // Get new size of buffer after clear

    SetConsoleMode(editor.hstdin, 0);

    while (1)
    {
        handleUserInput();
    }

    return 0;
}