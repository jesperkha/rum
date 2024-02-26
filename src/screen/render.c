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

static void drawLines(CharBuf *buf)
{
    for (int i = 0; i < buffer.textH; i++)
    {
        int row = i + buffer.cursor.offy;
        if (row >= buffer.numLines)
            break;

        CbColor(buf, colors.bg0, colors.bg2);

        if (buffer.cursor.row == row)
            CbColor(buf, colors.bg1, colors.yellow);

        // Line number
        char numbuf[12];
        // Assert short to avoid compiler error
        sprintf(numbuf, " %4d ", (short)(row + 1));
        CbAppend(buf, numbuf, 6);

        CbFg(buf, colors.fg0);

        // Line contents
        buffer.cursor.offx = max(buffer.cursor.col - buffer.textW + buffer.cursor.scrollDx, 0);
        int lineLength = buffer.lines[row].length - buffer.cursor.offx;
        int renderLength = min(lineLength, buffer.textW);
        char *lineBegin = buffer.lines[row].chars + buffer.cursor.offx;

        if (lineLength <= 0)
        {
            CbNextLine(buf);
            CbColorReset(buf);
            continue;
        }

        if (editor.config.syntaxEnabled && buffer.syntaxReady)
        {
            // Generate syntax highlighting for line and get new byte length
            int newLength;
            char *line = HighlightLine(lineBegin, renderLength, &newLength);
            CbAppend(buf, line, newLength);

            // Subtract added highlight strings from line length as they are 0-width
            int diff = newLength - lineLength;
            buf->lineLength -= diff;
        }
        else
            CbAppend(buf, lineBegin, renderLength);

        // Add padding at end for horizontal scroll
        int off = buffer.textW - lineLength;
        if (buffer.cursor.offx > 0 && off > 0)
            CbAppend(buf, padding, off);

        CbNextLine(buf);
        CbColorReset(buf);
    }
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
    CbRender(buf, 0, 0);
    MemFree(buf);
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
    CharBuf *buf = CbNew(editor.renderBuffer);

    // Draw every visible line in buffer
    drawLines(buf);

    // Draw squiggles for non-filled lines
    CbColor(buf, colors.bg0, colors.bg2);
    if (buffer.numLines < buffer.textH)
    {
        for (int i = 0; i < buffer.textH - buffer.numLines; i++)
        {
            CbAppend(buf, "~", 1);
            CbNextLine(buf);
        }
    }

    // Draw status line and command line
    drawStatusLine(buf);

    // Show welcome screen on empty buffers
    if (!buffer.dirty && !buffer.isFile)
        drawWelcomeScreen(buf);

    // Set cursor pos
    COORD pos = {buffer.cursor.col - buffer.cursor.offx + buffer.padX, buffer.cursor.row - buffer.cursor.offy};
    SetConsoleCursorPosition(editor.hbuffer, pos);
}