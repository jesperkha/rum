#include "wim.h"

extern Editor editor;
extern Colors colors;
extern Buffer buffer;

// Sets status bar info. Passing NULL for filename will leave the current one.
// Passing NULL for error will remove the current error. Call Render to update.
void SetStatus(char *filename, char *error)
{
    if (filename != NULL)
    {
        StrFilename(buffer.filepath, filename);
        strcpy(buffer.filepath, filename);

        char ext[8];
        StrFileExtension(ext, filename);
        EditorLoadSyntax(ext);
    }

    // if (error != NULL)
    //     strcpy(editor.info.error, error);

    // editor.info.hasError = error != NULL;
}

static void drawStatusLine(CharBuf *buf)
{
    // Draw status line and command line
    CbColor(buf, colors.fg0, colors.bg0);

    char *filename = buffer.filepath;
    CbAppend(buf, filename, strlen(filename));
    if (buffer.dirty && buffer.isFile)
        CbAppend(buf, "*", 1);

    CbColor(buf, colors.bg1, colors.fg0);
    CbNextLine(buf);

    // Command line
    CbColor(buf, colors.bg0, colors.fg0);

    // if (editor.info.hasError)
    // {
    //     CbColor(buf, colors.bg0, colors.red);
    //     char *error = editor.info.error;
    //     CbAppend(buf, "error: ", 7);
    //     CbAppend(buf, error, strlen(error));
    // }

    CbNextLine(buf);
    CbColorReset(buf);
}

static void drawWelcomeScreen(CharBuf *buf)
{
    char *lines[] = {
        TITLE,
        "github.com/jesperkha/wim",
        "",
        "Editor commands:",
        "exit         ctrl-q / :exit / <escape>",
        "open file    ctrl-o / :open [filename]",
        "save         ctrl-s / :save           ",
        "command      ctrl-c                   ",
        "new file     ctrl-n                   ",
        "delete line  ctrl-x                   ",
    };

    int numlines = sizeof(lines) / sizeof(lines[0]);
    int y = editor.height / 2 - numlines / 2;

    ScreenColor(colors.bg0, colors.blue);

    for (int i = 0; i < numlines; i++)
    {
        if (i == 1)
            ScreenFg(colors.fg0);
        if (i == 4)
            ScreenFg(colors.gray);

        char *text = lines[i];
        int pad = editor.width / 2 - strlen(text) / 2;
        ScreenWriteAt(pad, y + i, text);
    }
}

// Renders everything to the terminal. Sets cursor position. Shows welcome screen.
void Render()
{
    BufferRender(&buffer, 0, 0, editor.width, editor.height - 2);

    CharBuf *buf = CbNew(editor.renderBuffer);

    // Draw status line and command line
    drawStatusLine(buf);

    // Show welcome screen on empty buffers
    if (!buffer.dirty && !buffer.isFile)
        drawWelcomeScreen(buf);

    CbRender(buf, 0, editor.height - 2);
    MemFree(buf);

    // Set cursor pos
    COORD pos = {
        .X = buffer.cursor.col - buffer.cursor.offx + buffer.padX, //+ buffer.x,
        .Y = buffer.cursor.row - buffer.cursor.offy,               // + buffer.y,
    };
    SetConsoleCursorPosition(editor.hbuffer, pos);
}