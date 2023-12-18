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
int uiPromptYesNo(const char *message, bool select)
{
    int y = editorGetHandle()->height-1;
    int selected = select;
    cursorHide();

    while (true)
    {
        cursorTempPos(0, y);
        screenBufferClearLine(y);

        screenBufferWrite(BG(COL_RED), strlen(BG(COL_RED)));
        screenBufferWrite(FG(COL_FG0), strlen(FG(COL_FG0)));

        screenBufferWrite(message, strlen(message));
        screenBufferWrite(selected ?
            " "BG(COL_FG0)FG(COL_RED)"YES"BG(COL_RED)FG(COL_FG0) " NO" :
            " YES "BG(COL_FG0)FG(COL_RED) "NO"BG(COL_RED)FG(COL_FG0), 79);

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
            screenBufferClearLine(y);
            return selected ? UI_YES : UI_NO;
        }
    }
}

// Takes (x, y) of input pos and buffer to write string to. Size
// is the size of the buffer including the NULL terminator. Returns
// the prompt status: UI_OK or UI_CANCEL.
int uiTextInput(int x, int y, char *buffer, int size)
{
    char __buf[size];
    strcpy(__buf, buffer);
    int length = strlen(buffer);
    int minLen = length;

    while (true)
    {
        cursorHide();
        cursorTempPos(x, y);
        screenBufferClearLine(y);
        screenBufferWrite(BG(COL_BG0), strlen(BG(COL_BG0)));
        screenBufferWrite(FG(COL_FG0), strlen(FG(COL_FG0)));
        screenBufferWrite(__buf, length);
        cursorShow();

        char c;
        int keyCode;
        awaitInput(&c, &keyCode);

        switch (keyCode)
        {
        case K_ENTER:
            strcpy(buffer, __buf);
            screenBufferClearLine(y);
            return UI_OK;

        case K_ESCAPE:
            screenBufferClearLine(y);
            return UI_CANCEL;

        case K_BACKSPACE:
        {
            if (length > 0 && length > minLen)
                length--;
            break;
        }

        default:
            if (c < 32 || c > 126)
                continue;

            if (length < size - 1)
                __buf[length++] = c;
        }
    }
}