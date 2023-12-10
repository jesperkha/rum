#include "wim.h"

static void awaitInput(char *inputChar, int *keyCode)
{
    INPUT_RECORD record;
    DWORD read;

    while (true)
    {
        ReadConsoleInputA(GetStdHandle(STD_INPUT_HANDLE), &record, 1, &read);

        if (record.EventType != KEY_EVENT || !record.Event.KeyEvent.bKeyDown)
            continue;

        KEY_EVENT_RECORD event = record.Event.KeyEvent;
        *keyCode = (int)event.wVirtualKeyCode;
        *inputChar = event.uChar.AsciiChar;
        return;
    }
}

// Displays prompt message and hangs. Returns prompt status: UI_YES or UI_NO.
int uiPromptYesNo(const char *message)
{

    Editor *e = editorGetHandle();
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

        char c;
        int keyCode;
        awaitInput(&c, &keyCode);

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

// Takes (x, y) of input pos and buffer to write string to. Size
// is the size of the buffer including the NULL terminator. Returns
// the prompt status: UI_OK or UI_CANCEL.
int uiTextInput(int x, int y, char *buffer, int size)
{
    char __buf[size];
    int length = 0;

    cursorTempPos(x, y);

    while (true)
    {
        char c;
        int keyCode;
        awaitInput(&c, &keyCode);

        switch (keyCode)
        {
        case K_ENTER:
            strcpy(buffer, __buf);
            return UI_OK;

        case K_ESCAPE:
            return UI_CANCEL;

        case K_BACKSPACE:
        {
            if (length > 0)
                length--;
            break;
        }

        default:
            if (c < 32 || c > 126)
                continue;

            if (length < size - 1)
                __buf[length++] = c;
        }

        cursorHide();
        cursorTempPos(x, y);
        screenBufferClearLine(y);
        screenBufferWrite(__buf, length);
        cursorShow();
    }
}