#include "rum.h"

extern Editor editor;
extern Colors colors;

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
UiStatus UiPromptYesNo(char *message, bool select)
{
    int y = editor.height - 1;
    int selected = select;
    CursorHide();

    char cbuf[1024];
    CharBuf buf = CbNew(cbuf);

    while (true)
    {
        CbReset(&buf);
        CbColor(&buf, colors.red, colors.fg0);
        CbAppend(&buf, message, strlen(message));
        CbAppend(&buf, " ", 1);

        // bruh
        if (selected)
        {
            CbColor(&buf, colors.fg0, colors.red);
            CbAppend(&buf, "YES", 3);
            CbColor(&buf, colors.red, colors.fg0);
            CbAppend(&buf, " ", 1);
            CbAppend(&buf, "NO", 2);
        }
        else
        {
            CbColor(&buf, colors.red, colors.fg0);
            CbAppend(&buf, "YES", 3);
            CbAppend(&buf, " ", 1);
            CbColor(&buf, colors.fg0, colors.red);
            CbAppend(&buf, "NO", 2);
        }

        CbRender(&buf, 0, y);
        CursorHide();

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
            CursorShow();
            SetStatus(NULL, NULL);
            return selected ? UI_YES : UI_NO;
        }
    }
}

// Takes (x, y) of input pos and buffer to write string to. Size
// is the size of the buffer including the NULL terminator. Returns
// the prompt status: UI_OK or UI_CANCEL.
UiStatus UiTextInput(int x, int y, char *buffer, int size)
{
    char __buf[size];
    strcpy(__buf, buffer);
    int length = strlen(buffer);
    int minLen = length;

    char cbuf[1024];
    CharBuf buf = CbNew(cbuf);

    while (true)
    {
        CbReset(&buf);
        CbColor(&buf, colors.bg0, colors.fg0);
        CbAppend(&buf, __buf, length);
        CbNextLine(&buf);
        CbRender(&buf, x, y);
        CursorTempPos(x + length, y);

        char c;
        int keyCode;
        awaitInput(&c, &keyCode);

        switch (keyCode)
        {
        case K_ENTER:
            memset(__buf + length, 0, size - length);
            strcpy(buffer, __buf);
            SetStatus(NULL, NULL);
            return UI_OK;

        case K_ESCAPE:
            SetStatus(NULL, NULL);
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