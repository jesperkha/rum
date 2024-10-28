#include "rum.h"

extern Editor editor;
extern Colors colors;

char errorMsg[256];
bool hasError = false;

void SetError(char *error)
{
    if (error != NULL)
        strcpy(errorMsg, error);

    hasError = error != NULL;
}

static void drawCommandLine(CharBuf *buf)
{
    CbColor(buf, colors.bg0, colors.fg0);

    if (hasError)
    {
        CbColor(buf, colors.bg0, colors.keyword);
        CbAppend(buf, "error: ", 7);
        CbAppend(buf, errorMsg, strlen(errorMsg));
    }

    CbNextLine(buf);
    CbColorReset(buf);
}

static void drawWelcomeScreen()
{
    char *lines[] = {
        " _ __ _   _ _ __ ___  ",
        "| '__| | | | '_ ` _ \\ ",
        "| |  | |_| | | | | | |",
        "|_|   \\__,_|_| |_| |_|",
        "",
        TITLE,
        "github.com/jesperkha/rum",
        "",
        "Help   :help ",
        "Exit   ctrl-q",
        "Open   ctrl-o",
    };

    int numlines = sizeof(lines) / sizeof(lines[0]);
    int y = editor.height / 2 - numlines / 2;

    ScreenColor(colors.bg0, colors.fg0);

    for (int i = 0; i < numlines; i++)
    {
        if (i == 5)
            ScreenFg(colors.object);
        if (i == 6)
            ScreenFg(colors.fg0);
        if (i == 7)
            ScreenFg(colors.bracket);

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

    if (editor.splitBuffers)
        BufferRenderSplit(editor.buffers[editor.leftBuffer], editor.buffers[editor.rightBuffer]);
    else
        BufferRenderFull(editor.buffers[editor.leftBuffer]);

    // Command line
    CharBuf buf = CbNew(editor.renderBuffer);
    drawCommandLine(&buf);
    CbRender(&buf, 0, editor.height - 1);

    // Show welcome screen on empty buffers
    if (!curBuffer->dirty && !curBuffer->isFile && !editor.splitBuffers && !editor.uiOpen)
        drawWelcomeScreen();

    // Set cursor pos
    CursorUpdatePos();

    if (!curBuffer->cursor.visible)
        CursorHide();
}