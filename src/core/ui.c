#include "wim.h"

extern Editor editor;

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
int uiPromptYesNo(char *message, bool select)
{
    int y = editor.height-1;
    int selected = select;
    cursorHide();

    char cbuf[1024];
    CharBuffer buf = {
        .buffer = cbuf,
        .pos = cbuf,
    };

    while (true)
    {
        charbufClear(&buf);
        charbufBg(&buf, COL_RED);
        charbufFg(&buf, COL_FG0);
        charbufAppend(&buf, message, strlen(message));
        charbufAppend(&buf, " ", 1);

        // bruh
        if (selected)
        {
            charbufFg(&buf, COL_RED);
            charbufBg(&buf, COL_FG0);
            charbufAppend(&buf, "YES", 3);
            charbufBg(&buf, COL_RED);
            charbufFg(&buf, COL_FG0);
            charbufAppend(&buf, " ", 1);
            charbufAppend(&buf, "NO", 2);
        } else {
            charbufBg(&buf, COL_RED);
            charbufFg(&buf, COL_FG0);
            charbufAppend(&buf, "YES", 3);
            charbufAppend(&buf, " ", 1);
            charbufFg(&buf, COL_RED);
            charbufBg(&buf, COL_FG0);
            charbufAppend(&buf, "NO", 2);
        }

        charbufRender(&buf, 0, y);
        cursorHide();

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
            statusBarClear();
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

    char cbuf[1024];
    CharBuffer buf = {
        .buffer = cbuf,
        .pos = cbuf,
    };

    while (true)
    {
        charbufClear(&buf);
        charbufBg(&buf, COL_BG0);
        charbufFg(&buf, COL_FG0);
        charbufAppend(&buf, __buf, length);
        charbufNextLine(&buf);
        charbufRender(&buf, x, y);
        cursorTempPos(x + length, y);

        char c;
        int keyCode;
        awaitInput(&c, &keyCode);

        switch (keyCode)
        {
        case K_ENTER:
            memset(__buf+length, 0, size-length);
            strcpy(buffer, __buf);
            statusBarClear();
            return UI_OK;

        case K_ESCAPE:
            statusBarClear();
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