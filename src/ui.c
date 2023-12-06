#include <windows.h>
#include "editor.h"
#include "ui.h"

// Displays prompt message and hangs, returns true for yes, false for no.
int uiPromptYesNo(const char *message)
{
    INPUT_RECORD record;
    DWORD read;
    EditorHandle *e = editorGetHandle();
    int selected = false;

    cursorHide();
    screenBufferClearAll();

    int x = e->width / 2 - (strlen(message) + 16) / 2;
    int y = e->height / 2;

    while (true)
    {
        // Display prompt, swap out with box later
        cursorTempPos(x, y);
        screenBufferClearLine(y);
        screenBufferWrite(message, strlen(message));
        screenBufferWrite(selected ? " \033[47mYES\033[0m NO" : " YES \033[47mNO\033[0m", 16);

        // Wait for input key press
        if (!ReadConsoleInputA(GetStdHandle(STD_INPUT_HANDLE), &record, 1, &read) || read == 0)
            return_error("editorHandleInput() - Failed to read input");

        if (record.EventType != KEY_EVENT || !record.Event.KeyEvent.bKeyDown)
            continue;

        KEY_EVENT_RECORD event = record.Event.KeyEvent;
        WORD keyCode = event.wVirtualKeyCode;

        // Switch selected with left and right arrows
        // Confirm choice with enter and return select
        switch (keyCode)
        {
        case K_ARROW_LEFT:
            selected = true;
            break;

        case K_ARROW_RIGHT:
            selected = false;
            break;

        case K_ENTER:
            cursorShow();
            renderBufferBlank();
            return selected;
        }
    }
}