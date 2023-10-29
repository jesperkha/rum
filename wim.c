#include <windows.h>
#include <stdio.h>

#include "wim.h"

#define error(msg) printf("Error: %s\n", msg);
#define log(msg) printf("Log: %s\n", msg);

// Returns the editor key code of the key pressed, -1 on error.
int readInputKey(HANDLE fd)
{
    char buffer[8];
    DWORD read;
    if (!ReadFile(fd, buffer, 1, &read, NULL) || read == 0)
    {
        error("readInputKey() - Failed to read input");
        return -1;
    }

    return buffer[0];
}

int main(void)
{
    HANDLE term = GetStdHandle(STD_INPUT_HANDLE);
    SetConsoleMode(term, 0); // Set raw mode

    // Currently reads a single character from stdin and prints it out
    // Exits when q is pressed

    while (1)
    {
        int key = readInputKey(term);
        if (key == 'q')
            break;

        printf("%c", key);
    }

    return 0;
}