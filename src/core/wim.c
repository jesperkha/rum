#include "wim.h"

extern Editor editor;

// ---------------------- SCREEN BUFFER ----------------------

// Writes at cursor position
void screenBufferWrite(const char *string, int length)
{
    DWORD written;
    if (!WriteConsoleA(editor.hbuffer, string, length, &written, NULL) || written != length)
    {
        LogError("Failed to write to screen buffer");
        EditorExit();
    }
}

void screenBufferBg(int col)
{
    screenBufferWrite("\x1b[48;2;", 7);
    screenBufferWrite(editor.colors+col, 11);
    screenBufferWrite("m", 1);
}

void screenBufferFg(int col)
{
    screenBufferWrite("\x1b[38;2;", 7);
    screenBufferWrite(editor.colors+col, 11);
    screenBufferWrite("m", 1);
}

void screenBufferClearLine(int row)
{
    COORD pos = {0, row};
    DWORD written;
    FillConsoleOutputCharacterA(editor.hbuffer, ' ', editor.width, pos, &written);
}

void screenBufferClearAll()
{
    DWORD written;
    COORD pos = {0, 0};
    int size = editor.width * editor.height;
    FillConsoleOutputCharacterA(editor.hbuffer, ' ', size, pos, &written);
}

// ---------------------- CURSOR ----------------------

void cursorShow()
{
    CONSOLE_CURSOR_INFO info = {100, true};
    SetConsoleCursorInfo(editor.hbuffer, &info);
}

void cursorHide()
{
    CONSOLE_CURSOR_INFO info = {100, false};
    SetConsoleCursorInfo(editor.hbuffer, &info);
}

// Adds x,y to position
void cursorMove(int x, int y)
{
    cursorSetPos(editor.col + x, editor.row + y, true);
}

// KeepX is true when the cursor should keep the current max width
// when moving vertically, only really used with cursorMove.
void cursorSetPos(int x, int y, bool keepX)
{
    int dx = x - editor.col;
    int dy = y - editor.row;
    BufferScroll(dy); // Scroll by cursor offset

    editor.col = x;
    editor.row = y;

    Line line = editor.lines[editor.row];

    // Keep cursor within bounds
    if (editor.col < 0)
        editor.col = 0;
    if (editor.col > line.length)
        editor.col = line.length;
    if (editor.row < 0)
        editor.row = 0;
    if (editor.row > editor.numLines - 1)
        editor.row = editor.numLines - 1;
    if (editor.row - editor.offy > editor.textH)
        editor.row = editor.offy + editor.textH - editor.scrollDy;

    // Get indent for current line
    int i = 0;
    editor.indent = 0;
    while (i < editor.col && line.chars[i++] == ' ')
        editor.indent = i;

    // Keep cursor x when moving vertically
    if (keepX)
    {
        if (dy != 0)
        {
            if (editor.col > editor.colMax)
                editor.colMax = editor.col;
            if (editor.colMax <= line.length)
                editor.col = editor.colMax;
            if (editor.colMax > line.length)
                editor.col = line.length;
        }
        if (dx != 0)
            editor.colMax = editor.col;
    }
}

// Sets the cursor pos without additional stuff happening.
// The editor position is not updated so cursor returns to
// previous position when render is called.
void cursorTempPos(int x, int y)
{
    COORD pos = {x, y};
    SetConsoleCursorPosition(editor.hbuffer, pos);
}

// Restores cursor position to editor pos.
void cursorRestore()
{
    cursorSetPos(editor.col, editor.row, false);
}



// ---------------------- TYPING HELPERS ----------------------

const char begins[] = "\"'({[";
const char ends[] = "\"')}]";

void typingInsertTab()
{
    for (int i = 0; i < editor.config.tabSize; i++)
        BufferWrite(" ", 1);
}

// Matches braces, parens, strings etc with written char
void typingMatchParen(char c)
{
    Line line = editor.lines[editor.row];

    for (int i = 0; i < strlen(begins); i++)
    {
        if (c == ends[i] && line.chars[editor.col] == ends[i])
        {
            typingDeleteForward();
            break;
        }
        
        if (c == begins[i])
        {
            BufferWrite((char*)&ends[i], 1);
            cursorMove(-1, 0);
            break;
        }
    }
}

// When pressing enter after a paren, indent and move mathing paren to line below.
void typingBreakParen()
{
    Line line1 = editor.lines[editor.row];
    Line line2 = editor.lines[editor.row - 1];

    for (int i = 2; i < strlen(begins); i++)
    {
        char a = begins[i];
        char b = ends[i];

        if (line2.chars[line2.length-1] == a)
        {
            typingInsertTab();

            if (line1.chars[editor.col] == b)
            {
                BufferInsertLine(editor.row + 1);
                BufferSplitLineDown(editor.row);
            }

            return;
        }
    }
}

// Same as delete key on keyboard.
void typingDeleteForward()
{
    if (editor.col == editor.lines[editor.row].length)
    {
        if (editor.row == editor.numLines - 1)
            return;

        cursorHide();
        cursorSetPos(0, editor.row + 1, false);
    }
    else
    {
        cursorHide();
        cursorMove(1, 0);
    }

    BufferDeleteChar();
    cursorShow();
}

// ---------------------- RENDER ----------------------

#define color(bg, fg) CbColor(buf, bg, fg);
char padding[256] = {[0 ... 255] = ' '};

void renderBuffer()
{
    CharBuf *buf = CbNew(editor.renderBuffer);

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
            "exit       ctrl-q / :exit / <escape>",
            "command    ctrl-c                   ",
            "new file   ctrl-n                   ",
            "open file  ctrl-o / :open [filename]",
            "save       ctrl-s / :save           ",
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

// 100% effective for clearing screen. screenBufferClearAll may leave color
// artifacts sometimes, but is much faster.
void renderBufferBlank()
{
    cursorTempPos(0, 0);
    int size = editor.width * editor.height;
    memset(editor.renderBuffer, (int)' ', size);
    screenBufferWrite(editor.renderBuffer, size);
    cursorRestore();
}

// ---------------------- STATUS BAR ----------------------

// Does not update a field if left as NULL.
void statusBarUpdate(char *filename, char *error)
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
    renderBuffer();
}

void statusBarClear()
{
    statusBarUpdate(NULL, NULL);
}
