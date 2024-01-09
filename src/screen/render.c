#include "wim.h"

extern Editor editor;

// Used to pad shorter lines when scrolling horizontally
static char padding[256] = {[0 ... 255] = ' '};

// Sets status bar info. Passing NULL for filename will leave the current one.
// Passing NULL for error will remove the current error. Call Render to update.
void SetStatus(char *filename, char *error)
{
    if (filename != NULL)
    {
        // Get files basename
        char *slash = filename;
        for (int i = strlen(filename); i >= 0; i--)
        {
            if (filename[i] == '/' || filename[i] == '\\')
                break;

            slash = filename+i;
        }

        strcpy(editor.info.filename, slash);
        strcpy(editor.info.filepath, filename);
    }

    if (error != NULL)
        strcpy(editor.info.error, error);

    editor.info.hasError = error != NULL;
}

// Renders everything to the terminal. Sets cursor position. Shows welcome screen.
// Todo: extract render into smaller functions
void Render()
{
    CharBuf *buf = CbNew(editor.renderBuffer);
    #define color(bg, fg) CbColor(buf, bg, fg);

    // Draw lines
    for (int i = 0; i < editor.textH; i++)
    {
        int row = i + editor.offy;
        if (row >= editor.numLines)
            break;

        color(COL_BG0, COL_BG2);

        if (editor.row == row)
            color(COL_BG1, COL_YELLOW);

        // Line number
        char numbuf[12];
        // Assert short to avoid compiler error
        sprintf(numbuf, " %4d ", (short)(row + 1));
        CbAppend(buf, numbuf, 6);

        CbFg(buf, COL_FG0);

        // Line contents
        editor.offx = max(editor.col - editor.textW + editor.scrollDx, 0);
        int lineLength = editor.lines[row].length - editor.offx;
        int renderLength = min(lineLength, editor.textW);
        char *lineBegin = editor.lines[row].chars + editor.offx;

        if (lineLength <= 0)
        {
            CbNextLine(buf);
            CbColorReset(buf);
            continue;
        }

        if (editor.config.syntaxEnabled && editor.info.syntaxReady)
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
        int off = editor.textW - lineLength;
        if (editor.offx > 0 && off > 0)
            CbAppend(buf, padding, off);

        CbNextLine(buf);
        CbColorReset(buf);
    }

    color(COL_BG0, COL_BG2);

    // Draw squiggles for non-filled lines
    if (editor.numLines < editor.textH)
        for (int i = 0; i < editor.textH - editor.numLines; i++)
        {
            CbAppend(buf, "~", 1);
            CbNextLine(buf);
        }

    // Draw status line and command line

    color(COL_FG0, COL_BG0);

    char *filename = editor.info.filename;
    CbAppend(buf, filename, strlen(filename));
    if (editor.info.dirty && editor.info.fileOpen)
        CbAppend(buf, "*", 1);

    color(COL_BG1, COL_FG0);
    CbNextLine(buf);

    // Command line

    color(COL_BG0, COL_FG0);

    if (editor.info.hasError)
    {
        color(COL_BG0, COL_RED);
        char *error = editor.info.error;
        CbAppend(buf, "error: ", 7);
        CbAppend(buf, error, strlen(error));
    }

    CbNextLine(buf);
    CbColorReset(buf);
    CbRender(buf, 0, 0);
    memFree(buf);

    // Show info screen on empty buffer
    if (!editor.info.dirty && !editor.info.fileOpen)
    {
        char *lines[] = {
            TITLE,
            "github.com/jesperkha/wim",
            "last updated "UPDATED,
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
        int y = editor.height/2 - numlines/2;

        screenBufferBg(COL_BG0);
        screenBufferFg(COL_BLUE);

        for (int i = 0; i < numlines; i++)
        {
            if (i == 1)
                screenBufferFg(COL_FG0);
            if (i == 5)
                screenBufferFg(COL_GREY);

            char *text = lines[i];
            int pad = editor.width/2 - strlen(text)/2;
            editorWriteAt(pad, y + i, text);
        }
    }

    // Set cursor pos
    COORD pos = {editor.col - editor.offx + editor.padH, editor.row - editor.offy};
    SetConsoleCursorPosition(editor.hbuffer, pos);
}