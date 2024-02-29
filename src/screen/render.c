#include "wim.h"

extern Editor editor;
extern Colors colors;
extern Buffer buffer;

// Returns pointer to highlight buffer. Must NOT be freed. Line is the
// pointer to the line contents and the length is excluding the NULL
// terminator. Writes byte length of highlighted text to newLength.
char *HighlightLine(char *line, int lineLength, int *newLength);

// Used to pad shorter lines when scrolling horizontally
static char padding[256] = {[0 ... 255] = ' '};

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

static void drawBuffer(Buffer *b, int x, int y)
{
    b->syntaxReady = true;

    int textW = editor.width - b->padX;
    int textH = editor.height - b->padY - 2;
    b->textH = textH;

    CursorHide();

    HANDLE H = editor.hbuffer;

    for (int i = 0; i < textH; i++)
    {
        int row = i + b->cursor.offy;

        if (row >= b->numLines || y + i >= editor.height)
            break;

        Line line = b->lines[row];
        SetConsoleCursorPosition(H, (COORD){x, y + i});

        // Line background color
        if (b->cursor.row == row)
            ScreenColor(colors.bg1, colors.yellow);
        else
            ScreenColor(colors.bg0, colors.bg2);

        // Line numbers
        char numbuf[12];
        sprintf(numbuf, " %4d ", (short)(row + 1));
        WriteConsoleA(H, numbuf, b->padX, NULL, NULL);

        // Line contents
        ScreenFg(colors.fg0);
        b->cursor.offx = max(b->cursor.col - textW + b->cursor.scrollDx, 0);
        int lineLength = line.length - b->cursor.offx;

        int renderLength = max(min(min(lineLength, textW), editor.width), 0);
        char *lineBegin = line.chars + b->cursor.offx;

        if (editor.config.syntaxEnabled && b->syntaxReady)
        {
            // Generate syntax highlighting for line and get new byte length
            int newLength;
            char *line = HighlightLine(lineBegin, renderLength, &newLength);
            WriteConsoleA(H, line, newLength, NULL, NULL);
        }
        else
            WriteConsoleA(H, lineBegin, renderLength, NULL, NULL);

        // Padding after
        if (renderLength < textW)
            WriteConsoleA(H, padding, textW - renderLength, NULL, NULL);
    }

    // Draw squiggles for non-filled lines
    ScreenColor(colors.bg0, colors.bg2);
    if (b->numLines < b->textH)
    {
        for (int i = 0; i < b->textH - b->numLines; i++)
        {
            ScreenWrite("~", 1);
            ScreenWrite(padding, editor.width - 1);
        }
    }

    CursorShow();
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
    drawBuffer(&buffer, 0, 0);

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