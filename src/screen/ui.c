#include "rum.h"

extern Editor editor;
extern Colors colors;

char bars[] = {
    (char)179, // Vertical
    (char)196, // Horizontal + 1
    (char)218, // Top left + 2
    (char)191, // Top right + 3
    (char)192, // Bottom left + 4
    (char)217, // Bottom right + 5
};

// Draws border from x to width, y to height. Returns new width if set by title.
int drawBorder(int x, int y, int width, int height, char *title)
{
    int titleLen = 0;

    if (title != NULL)
    {
        titleLen = strlen(title);
        width = max(width, titleLen);
    }

    CharBuf cb = CbNew(editor.renderBuffer);

    // Top bar
    CbColor(&cb, colors.bg0, colors.fg0);
    CbAppend(&cb, bars + 2, 1);
    if (title != NULL)
    {
        // Title
        CbAppend(&cb, " ", 1);
        CbColor(&cb, colors.bg0, colors.string);
        CbAppend(&cb, title, titleLen);
        CbColor(&cb, colors.bg0, colors.fg0);
        CbAppend(&cb, " ", 1);
        CbRepeat(&cb, *(bars + 1), width - titleLen - 4);
    }
    else
        CbRepeat(&cb, *(bars + 1), width - 2);
    CbAppend(&cb, bars + 3, 1);
    CbRender(&cb, x, y);
    CbReset(&cb);

    // Side walls
    CbColor(&cb, colors.bg0, colors.fg0);
    for (int i = 0; i < height - 2; i++)
    {
        CbAppend(&cb, bars, 1);
        CbRepeat(&cb, ' ', width - 2);
        CbAppend(&cb, bars, 1);
        CbRender(&cb, x, y + i + 1);
        CbReset(&cb);
    }

    // Bottom bar
    CbColor(&cb, colors.bg0, colors.fg0);
    CbAppend(&cb, bars + 4, 1);
    CbRepeat(&cb, *(bars + 1), width - 2);
    CbAppend(&cb, bars + 5, 1);
    CbRender(&cb, x, y + height - 1);
    CbReset(&cb);

    return width;
}

// Displays prompt message and hangs. Returns prompt status: UI_YES or UI_NO.
UiStatus UiPromptYesNo(char *message, bool select)
{
    int selected = select;
    int messageLen = strlen(message);

    editor.uiOpen = true;
    Render();
    editor.uiOpen = false;

    CharBuf cb = CbNew(editor.renderBuffer);

    int x = curBuffer->width / 2 - messageLen / 2;
    int y = curBuffer->height / 2 - 4;

    while (true)
    {
        drawBorder(x, y, messageLen + 4, 4, NULL);

        ScreenColor(colors.bg0, colors.fg0);
        ScreenWriteAt(x + 2, y + 1, message);

        CbBg(&cb, selected ? colors.fg0 : colors.bg0);
        CbColorWord(&cb, selected ? colors.bg0 : colors.fg0, "YES", 3);
        CbBg(&cb, colors.bg0);
        CbAppend(&cb, " ", 1);
        CbBg(&cb, !selected ? colors.fg0 : colors.bg0);
        CbColorWord(&cb, !selected ? colors.bg0 : colors.fg0, "NO", 2);

        CbRender(&cb, max(x + messageLen / 2 - 3 + 2, 0), y + 2);
        CbReset(&cb);
        CursorHide();

        InputInfo info;
        EditorReadInput(&info);

        if (info.eventType == INPUT_KEYDOWN)
        {
            if (info.keyCode == K_ARROW_LEFT)
                selected = true;
            else if (info.keyCode == K_ARROW_RIGHT)
                selected = false;
            else if (info.keyCode == K_ENTER)
                break;
        }
    }

    return selected ? UI_YES : UI_NO;
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
        {
            char c = info.asciiChar;
            if (c < 32 || c > 126)
                continue;
            if (res.length < maxSize - 1) // -1 to leave room for NULL
                res.buffer[res.length++] = c;
        }
        }
    }
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
        int w = drawBorder(x - 1, y - 1, width + 2, numItems + 2, prompt);

        for (int i = 0; i < numItems; i++)
        {
            int length = strlen(items[i]);
            CursorTempPos(x, y + i);
            if (i == selected)
                ScreenColor(colors.fg0, colors.bg0);
            else
                ScreenColor(colors.bg0, colors.fg0);
            ScreenWrite(items[i], length);
            ScreenWrite(editor.padBuffer, w - length - 2);
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

void UiShowCompletion(char **items, int numItems, int selected)
{
    int x = curBuffer->cursor.col + curBuffer->offX + curBuffer->padX;
    int y = curBuffer->cursor.row + 1;
    int w = 20;

    for (int i = 0; i < numItems; i++)
    {
        i == selected
            ? ScreenColor(colors.bg2, colors.fg0)
            : ScreenColor(colors.bg1, colors.fg0);

        ScreenWriteAt(x, y + i, items[i]);
        ScreenWrite(editor.padBuffer, w - strlen(items[i]));
    }

    CursorUpdatePos();
}

void UiTextbox(const char *text)
{
    editor.uiOpen = true;
    Render();
    editor.uiOpen = false;

    int width = curBuffer->width - 4;
    int x = 2 + curBuffer->offX;
    int y = 1;

    int textX = x + 2;
    int textY = y + 1;
    int textW = width - 4;

    char *textPtr = strdup(text);
    StrCapWidth(textPtr, textW);

    int numLines = StrCount(textPtr, '\n') + 1;
    int height = min(numLines + 2, curBuffer->textH - 2);
    int textH = height - 2;

    int scrollY = 0; // Scroll offset input by user

    while (true)
    {
        if (numLines > textH)
            drawBorder(x, y, width, height, "Close: <enter> | Scroll: <arrows> or <j, k>");
        else
            drawBorder(x, y, width, height, "Close: <enter>");

        CursorHide();

        char *lineIterPtr = textPtr;
        char *lineBegin = NULL;
        int length = 0;

        int line = 0;

        for (int i = 0; i < scrollY; i++) // Safe, splitnext handles null input
            lineIterPtr = StrSplitNext(lineIterPtr, '\n', &lineBegin, &length);

        while (line < textH)
        {
            lineIterPtr = StrSplitNext(lineIterPtr, '\n', &lineBegin, &length);
            if (lineBegin == NULL)
                break;

            CursorTempPos(textX, textY + line);
            ScreenWrite(lineBegin, min(length, textW));
            line++;
        }

        InputInfo info;
        EditorReadInput(&info);
        if (info.eventType == INPUT_KEYDOWN)
        {
            if (info.keyCode == K_ENTER)
                break;

            if ((info.keyCode == K_ARROW_DOWN || info.asciiChar == 'j') && scrollY < numLines - textH / 2)
                scrollY++;
            if ((info.keyCode == K_ARROW_UP || info.asciiChar == 'k') && scrollY > 0)
                scrollY--;
        }
    }

    free(textPtr);
}

UiStatus UiInputBox(char *prompt, char *outBuf, int *outLen, int maxLen)
{
    if (*outLen == 0)
        memset(outBuf, 0, maxLen);

    int x = curBuffer->offX;
    drawBorder(x, 0, curBuffer->width, 3, prompt);
    ScreenWriteAt(x + 2, 1, outBuf);

    InputInfo info;
    while (true)
    {
        EditorReadInput(&info);
        if (info.eventType == INPUT_KEYDOWN)
            break;
    }

    if (info.keyCode == K_ESCAPE)
        return UI_CANCEL;
    else if (info.keyCode == K_ENTER)
        return UI_OK;
    else if (info.keyCode == K_BACKSPACE)
    {
        if (*outLen > 0)
            outBuf[--(*outLen)] = 0;
        return UI_CONTINUE;
    }

    if (isChar(info.asciiChar))
    {
        if (*outLen < maxLen - 1)
            outBuf[(*outLen)++] = info.asciiChar;
        return UI_CONTINUE;
    }

    return UI_CONTINUE;
}