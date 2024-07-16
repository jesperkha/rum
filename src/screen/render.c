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

static void drawCommandLine(CharBuf *buf)
{
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

static void drawWelcomeScreen()
{
    char *lines[] = {
        TITLE,
        "github.com/jesperkha/rum",
        "",
        "Exit   ESC   ",
        "Help   :help",
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
    COORD pos = {
        .X = curBuffer->cursor.col - curBuffer->cursor.offx + curBuffer->padX + curBuffer->offX,
        .Y = curBuffer->cursor.row - curBuffer->cursor.offy + curBuffer->padY, // + curBuffer->y,
    };
    SetConsoleCursorPosition(editor.hbuffer, pos);
}