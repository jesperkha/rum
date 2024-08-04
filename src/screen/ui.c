#include "rum.h"

extern Editor editor;
extern Colors colors;

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

        InputInfo info;
        EditorReadInput(&info);

        // Switch selected with left and right arrows
        // Confirm choice with enter and return select
        switch (info.keyCode)
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

        default:
            break;
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

// Draws border *around* area given. Returns new width if set by title.
int drawBorder(int x, int y, int width, int height, char *title)
{
    char bars[] = {
        179, // Vertical
        196, // Horizontal
        218, // Top left
        191, // Top right
        192, // Bottom left
        217, // Bottom right
    };

    int titleLen = 0;

    if (title != NULL)
    {
        titleLen = strlen(title);
        width = max(width, titleLen);
    }

    CharBuf cb = CbNew(editor.renderBuffer);

    // Top bar
    CbColor(&cb, colors.bg0, colors.fg0);
    CbAppend(&cb, " ", 1);
    CbAppend(&cb, bars + 2, 1);
    if (title != NULL)
    {
        // Title
        CbAppend(&cb, " ", 1);
        CbColor(&cb, colors.bg0, colors.green);
        CbAppend(&cb, title, titleLen);
        CbColor(&cb, colors.bg0, colors.fg0);
        CbAppend(&cb, " ", 1);
        CbRepeat(&cb, *(bars + 1), width - titleLen);
    }
    else
        CbRepeat(&cb, *(bars + 1), width + 2);
    CbAppend(&cb, bars + 3, 1);
    CbAppend(&cb, " ", 1);
    CbRender(&cb, x - 3, y - 1);
    CbReset(&cb);

    // Side walls
    CbColor(&cb, colors.bg0, colors.fg0);
    for (int i = 0; i < height; i++)
    {
        CbAppend(&cb, " ", 1);
        CbAppend(&cb, bars, 1);
        CbRepeat(&cb, ' ', width + 2);
        CbAppend(&cb, bars, 1);
        CbAppend(&cb, " ", 1);
        CbRender(&cb, x - 3, y + i);
        CbReset(&cb);
    }

    // Bottom bar
    CbColor(&cb, colors.bg0, colors.fg0);
    CbAppend(&cb, " ", 1);
    CbAppend(&cb, bars + 4, 1);
    CbRepeat(&cb, *(bars + 1), width + 2);
    CbAppend(&cb, bars + 5, 1);
    CbAppend(&cb, " ", 1);
    CbRender(&cb, x - 3, y + height);
    CbReset(&cb);

    return width;
}

UiResult UiPromptList(char **items, int numItems, char *prompt)
{
    return UiPromptListEx(items, numItems, prompt, 0);
}

UiResult UiPromptListEx(char **items, int numItems, char *prompt, int startIdx)
{
    int width = min(curBuffer->width / 2, 30);
    int y = editor.height / 2 - numItems / 2;
    int x = curBuffer->offX + curBuffer->width / 2 - width / 2 - 1;
    int selected = startIdx;

    editor.uiOpen = true;
    Render();

    while (true)
    {
        int w = drawBorder(x, y, width, numItems, prompt);

        for (int i = 0; i < numItems; i++)
        {
            int length = strlen(items[i]);
            CursorTempPos(x, y + i);
            if (i == selected)
                ScreenColor(colors.fg0, colors.bg0);
            else
                ScreenColor(colors.bg0, colors.fg0);
            ScreenWrite(items[i], length);
            ScreenWrite(editor.padBuffer, w - length);
        }

        CursorHide();

        InputInfo info;
        EditorReadInput(&info);

        if (info.eventType != INPUT_KEYDOWN)
            continue;

        bool moveDown = info.keyCode == K_ARROW_DOWN || info.asciiChar == 'j';
        bool moveUp = info.keyCode == K_ARROW_UP || info.asciiChar == 'k';
        bool choose = info.keyCode == K_ENTER || info.asciiChar == ' ';

        if (info.keyCode == K_ESCAPE)
        {
            editor.uiOpen = false;
            return (UiResult){.status = UI_CANCEL};
        }
        else if (choose)
        {
            editor.uiOpen = false;
            return (UiResult){.status = UI_OK, .choice = selected};
        }
        else if (moveUp)
        {
            selected--;
            if (selected < 0)
                selected = 0;
        }
        else if (moveDown)
        {
            selected++;
            if (selected >= numItems)
                selected = numItems - 1;
        }
    }

    return (UiResult){.status = UI_OK};
}