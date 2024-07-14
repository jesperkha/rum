#include "rum.h"

extern Editor editor;
extern Colors colors;

char errorMsg[256];
bool hasError = false;

// Sets status bar info. Passing NULL for filename will leave the current one.
// Passing NULL for error will remove the current error. Call Render to update.
void SetStatus(char *filename, char *error)
{
    if (error != NULL)
        strcpy(errorMsg, error);

    hasError = error != NULL;
}

static void drawStatusLine(CharBuf *buf)
{
    // Draw status line and command line
    CbColor(buf, colors.fg0, colors.bg0);
    if (editor.mode == MODE_VIM)
        CbAppend(buf, "EDIT", 4);
    else if (editor.mode == MODE_INSERT)
        CbAppend(buf, "INSERT", 6);

    CbColor(buf, colors.bg1, colors.fg0);
    CbAppend(buf, " ", 1);

    if (curBuffer->readOnly)
    {
        CbAppend(buf, "Open: ", 6);
        CbAppend(buf, curBuffer->filepath, strlen(curBuffer->filepath));
        CbColor(buf, colors.bg1, colors.red);
        CbAppend(buf, " (READ-ONLY)", 12);
    }
    else if (curBuffer->isFile)
    {
        CbAppend(buf, "Open: ", 6);
        CbAppend(buf, curBuffer->filepath, strlen(curBuffer->filepath));
        if (curBuffer->dirty && curBuffer->isFile && !curBuffer->readOnly)
            CbAppend(buf, "*", 1);
    }
    else
        CbAppend(buf, "[empty]", 7);

    CbColor(buf, colors.bg1, colors.fg0);
    CbNextLine(buf);

    // Command line
    CbColor(buf, colors.bg0, colors.fg0);

    if (hasError)
    {
        CbColor(buf, colors.bg0, colors.red);
        CbAppend(buf, "error: ", 7);
        CbAppend(buf, errorMsg, strlen(errorMsg));
    }

    CbNextLine(buf);
    CbColorReset(buf);
}

static void drawWelcomeScreen(CharBuf *buf)
{
    char *lines[] = {
        TITLE,
        "github.com/jesperkha/rum",
        "",
        "Exit   ESC   ",
        "Help   ctrl-h",
        "Open   ctrl-o",
    };

    int numlines = sizeof(lines) / sizeof(lines[0]);
    int y = editor.height / 2 - numlines / 2;

    ScreenColor(colors.bg0, colors.blue);

    for (int i = 0; i < numlines; i++)
    {
        if (i == 1)
            ScreenFg(colors.fg0);
        if (i == 2)
            ScreenFg(colors.gray);

        char *text = lines[i];
        int pad = editor.width / 2 - strlen(text) / 2;
        ScreenWriteAt(pad, y + i, text);
    }
}

// Renders everything to the terminal. Sets cursor position. Shows welcome screen.
void Render()
{
    if (editor.hbuffer == INVALID_HANDLE_VALUE)
        Error("Render called before csb init");

    BufferRender(curBuffer, 0, 0, editor.width, editor.height - 2);

    CharBuf buf = CbNew(editor.renderBuffer);

    // Draw status line and command line
    drawStatusLine(&buf);

    // Show welcome screen on empty buffers
    if (!curBuffer->dirty && !curBuffer->isFile)
        drawWelcomeScreen(&buf);

    CbRender(&buf, 0, editor.height - 2);

    // Set cursor pos
    COORD pos = {
        .X = curBuffer->cursor.col - curBuffer->cursor.offx + curBuffer->padX, //+ curBuffer->x,
        .Y = curBuffer->cursor.row - curBuffer->cursor.offy,                   // + curBuffer->y,
    };
    SetConsoleCursorPosition(editor.hbuffer, pos);
}