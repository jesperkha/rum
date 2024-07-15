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

void UiFreeResult(UiResult res)
{
    MemFree(res.buffer);
}

UiResult UiGetTextInput(char *prompt, int maxSize)
{
    char cbuf[1024];
    CharBuf buf = CbNew(cbuf);

    UiResult res = {
        .maxLength = maxSize,
        .length = 0,
        .buffer = MemZeroAlloc(maxSize),
        .status = UI_OK,
    };
    AssertNotNull(res.buffer);

    int promptLen = strlen(prompt);

    while (true)
    {
        CbReset(&buf);
        CbColor(&buf, colors.bg0, colors.fg0);
        CbAppend(&buf, prompt, promptLen);
        CbAppend(&buf, res.buffer, res.length);
        CbNextLine(&buf);
        CbRender(&buf, 0, editor.height - 1);
        CursorTempPos(res.length + promptLen, editor.height - 1);

        InputInfo info;
        EditorReadInput(&info);

        if (info.eventType != INPUT_KEYDOWN)
            continue;

        switch (info.keyCode)
        {
        case K_ENTER:
            return res;

        case K_ESCAPE:
            return (UiResult){.status = UI_CANCEL};

        case K_BACKSPACE:
        {
            if (res.length > 0)
                res.buffer[--res.length] = 0;
            break;
        }

        default:
            char c = info.asciiChar;
            if (c < 32 || c > 126)
                continue;
            if (res.length < maxSize - 1) // -1 to leave room for NULL
                res.buffer[res.length++] = c;
        }
    }
}