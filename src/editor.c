#include "wim.h"
#include "util.h"

Editor editor; // Global for convenience

Editor *editorGetHandle()
{
    return &editor;
}

// Populates editor global struct and creates empty file buffer. Exits on error.
void editorInit()
{
    system("color");

    editor.hstdin = GetStdHandle(STD_INPUT_HANDLE);
    editor.hbuffer = CreateConsoleScreenBuffer(
        GENERIC_WRITE | GENERIC_READ, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);

    if (editor.hbuffer == INVALID_HANDLE_VALUE || editor.hstdin == INVALID_HANDLE_VALUE)
    {
        logError("editorInit() - Failed to get one or more handles");
        ExitProcess(EXIT_FAILURE);
    }

    SetConsoleActiveScreenBuffer(editor.hbuffer); // Swap buffer
    SetConsoleMode(editor.hstdin, 0);             // Set raw input mode
    SetConsoleTitleA(TITLE);
    FlushConsoleInputBuffer(editor.hstdin);

    editorUpdateSize();
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(editor.hbuffer, &info);
    editor.initSize = (COORD){info.srWindow.Right, info.srWindow.Bottom};

    editor.offx = 0;
    editor.offy = 0;
    editor.scrollDx = 5;
    editor.scrollDy = 5;

    editor.config = (Config){
        .matchParen = true,
        .syntaxEnabled = true,
    };

    editor.numLines = 0;
    editor.lineCap = BUFFER_LINE_CAP;
    editor.lines = calloc(editor.lineCap, sizeof(Line));

    COORD maxSize = GetLargestConsoleWindowSize(editor.hbuffer);
    editor.renderBuffer = malloc(maxSize.X * maxSize.Y * 4);

    check_pointer(editor.lines, "bufferInit");
    check_pointer(editor.renderBuffer, "bufferInit");

    bufferCreateLine(0);
    renderBuffer();
    statusBarUpdate("[empty file]");
}

// Free, clean, and exit
void editorExit()
{
    for (int i = 0; i < editor.numLines; i++)
        free(editor.lines[i].chars);

    free(editor.lines);
    free(editor.renderBuffer);
    SetConsoleScreenBufferSize(editor.hbuffer, editor.initSize);
    CloseHandle(editor.hbuffer);
    ExitProcess(EXIT_SUCCESS);
}

// Update editor and screen buffer size.
void editorUpdateSize()
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(editor.hbuffer, &info);

    short bufferW = info.dwSize.X;
    short windowH = info.srWindow.Bottom - info.srWindow.Top + 1;

    // Remove scrollbar by setting buffer height to window height
    COORD newSize;
    newSize.X = bufferW;
    newSize.Y = windowH;
    SetConsoleScreenBufferSize(editor.hbuffer, newSize);

    editor.width = (int)(newSize.X);
    editor.height = (int)(newSize.Y);

    editor.padH = 6; // Line numbers
    editor.padV = 2; // Status line

    editor.textW = editor.width - editor.padH;
    editor.textH = editor.height - editor.padV;
}

// Writes text at given x, y.
void editorWriteAt(int x, int y, const char *text)
{
    cursorHide();
    cursorTempPos(x, y);
    screenBufferWrite(text, strlen(text));
    cursorRestore();
    cursorShow();
}

// Takes action based on current user input. Returns -1 on error.
int editorHandleInput()
{
    INPUT_RECORD record;
    DWORD read;

    if (!ReadConsoleInputA(editor.hstdin, &record, 1, &read) || read == 0)
    {
        // Set status bar error message
        // return_error("editorHandleInput() - Failed to read input");
        return RETURN_ERROR;
    }

    if (record.EventType == WINDOW_BUFFER_SIZE_EVENT)
    {
        editorUpdateSize();
        renderBuffer();
        statusBarRender();
        return RETURN_SUCCESS;
    }

    if (record.EventType == KEY_EVENT)
    {
        if (record.Event.KeyEvent.bKeyDown)
        {
            KEY_EVENT_RECORD event = record.Event.KeyEvent;
            WORD keyCode = event.wVirtualKeyCode;
            char inputChar = event.uChar.AsciiChar;

            switch (keyCode)
            {
            // Debug key for testing
            case K_PAGEDOWN:
                break;

            case K_ESCAPE:
                editorExit();

            case K_BACKSPACE:
                bufferDeleteChar();
                break;

            case K_DELETE:
                typingDeleteForward();
                break;

            case K_ENTER:
                bufferInsertLine(editor.row + 1);
                int length = editor.lines[editor.row + 1].length;
                bufferSplitLineDown(editor.row);
                cursorSetPos(length, editor.row + 1);
                if (editor.config.matchParen)
                    typingBreakParen();
                break;

            case K_TAB:
                bufferWriteChar(' ');
                bufferWriteChar(' ');
                bufferWriteChar(' ');
                bufferWriteChar(' ');
                break;

            case K_ARROW_UP:
                cursorMove(0, -1);
                break;

            case K_ARROW_DOWN:
                cursorMove(0, 1);
                break;

            case K_ARROW_LEFT:
                cursorMove(-1, 0);
                break;

            case K_ARROW_RIGHT:
                cursorMove(1, 0);
                break;

            default:
                bufferWriteChar(inputChar);
                if (editor.config.matchParen)
                    typingMatchParen(inputChar);
            }

            renderBuffer();
        }
    }

    return RETURN_SUCCESS;
}

// Helper, creates line in linebuf and writes line content
static void writeLineToBuffer(int row, char *buffer, int length)
{
    // Realloc buffer line array if full
    if (editor.numLines >= editor.lineCap)
    {
        // Realloc editor line buffer array when full
        editor.lineCap += BUFFER_LINE_CAP;
        editor.lines = realloc(editor.lines, editor.lineCap * sizeof(Line));
        check_pointer(editor.lines, "bufferInsertLine");
    }

    Line line = {
        .row = row,
        .length = length - 1,
        .idx = 0,
    };

    // Calculate cap size for the line length
    int l = DEFAULT_LINE_LENGTH;
    int cap = (length / l) * l + l;

    // Allocate chars and copy over line
    char *chars = calloc(cap, sizeof(char));
    check_pointer(chars, "editorOpenFile");
    strncpy(chars, buffer, length - 1);

    // Fill out line values and copy line to line array
    line.cap = cap;
    line.chars = chars;
    memcpy(&editor.lines[row], &line, sizeof(Line));

    // Increment number of line, position in buffer, and row
    editor.numLines = row + 1;
}

// Loads file into buffer. Filepath must either be an absolute path
// or name of a file in the same directory as wim.
int editorLoadFile(char *filepath)
{
    // Open file. editorLoadFile does not create files and fails on file-not-found
    HANDLE file = CreateFileA(filepath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        logError("failed to load file");
        return RETURN_ERROR;
    }

    // Get file size and read file contents into string buffer
    DWORD size = GetFileSize(file, NULL);
    DWORD read;
    char buffer[size];
    if (!ReadFile(file, buffer, size, &read, NULL))
    {
        logError("failed to read file");
        CloseHandle(file);
        return RETURN_ERROR;
    }

    CloseHandle(file);

    // Read file line by line
    char *newline;
    char *ptr = buffer;
    int row = 0;

    while ((newline = strstr(ptr, "\n")) != NULL)
    {
        // Get distance from current pos in buffer and found newline
        // Then strncpy the line into the line char buffer
        int length = newline - ptr;
        writeLineToBuffer(row, ptr, length);
        ptr += length + 1;
        row++;
    }

    // Write last line of file
    writeLineToBuffer(row, ptr, size - (ptr - buffer) + 1);

    renderBuffer();
    statusBarUpdate(filepath);
    return RETURN_SUCCESS;
}

// Writes content of buffer to filepath. Does not create file.
int editorSaveFile(char *filepath)
{
    // Accumulate size of buffer by line length
    int size = 0;
    for (int i = 0; i < editor.numLines; i++)
        size += editor.lines[i].length + 1; // +newline

    char buffer[size];
    char *ptr = buffer;

    // Write to buffer, add newline for each line
    for (int i = 0; i < editor.numLines; i++)
    {
        Line line = editor.lines[i];
        memcpy(ptr, line.chars, line.length);
        ptr += line.length;
        *ptr = '\n';
        ptr++;
    }

    // Open file - does not create new file
    HANDLE file = CreateFileA(filepath, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        logError("failed to open file");
        return RETURN_ERROR;
    }

    DWORD written;
    if (!WriteFile(file, buffer, size - 1, &written, NULL))
    {
        logError("failed to write to file");
        CloseHandle(file);
        return RETURN_ERROR;
    }

    CloseHandle(file);
    return RETURN_SUCCESS;
}

// ---------------------- SCREEN BUFFER ----------------------

// Write line to screen buffer
void screenBufferWrite(const char *string, int length)
{
    DWORD written;
    if (!WriteConsoleA(editor.hbuffer, string, length, &written, NULL) || written != length)
    {
        logError("Failed to write to screen buffer");
        editorExit();
    }
}

// Fills line with blanks
void screenBufferClearLine(int row)
{
    COORD pos = {0, row};
    DWORD written;
    FillConsoleOutputCharacterA(editor.hbuffer, (WCHAR)' ', editor.width, pos, &written);
}

// Clears the whole buffer
void screenBufferClearAll()
{
    DWORD written;
    COORD pos = {0, 0};
    int size = editor.width * editor.height;
    FillConsoleOutputCharacterA(editor.hbuffer, (WCHAR)' ', size, pos, &written);
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

// Adds x, y to cursor position. Updates editor cursor position.
void cursorMove(int x, int y)
{
    cursorSetPos(editor.col + x, editor.row + y);
}

// Sets the cursor position to x, y. Updates editor cursor position.
void cursorSetPos(int x, int y)
{
    int dx = x - editor.col;
    int dy = y - editor.row;
    bufferScroll(dx, dy); // Scroll by cursor offset

    editor.col = x;
    editor.row = y;

    Line line = editor.lines[editor.row];

    // Cursor out of bounds
    if (editor.col < 0)
        editor.col = 0;
    if (editor.col > line.length)
        editor.col = line.length;
    if (editor.row < 0)
        editor.row = 0;
    if (editor.row > editor.numLines - 1)
        editor.row = editor.numLines - 1;

    // Set line indent
    int i = 0;
    editor.indent = 0;
    while (i < line.length && line.chars[i++] == ' ')
        editor.indent = i;

    // Keep cursor X when moving down
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

    COORD pos = {editor.col - editor.offx + editor.padH, editor.row - editor.offy};
    SetConsoleCursorPosition(editor.hbuffer, pos);
}

// Sets the cursor pos, does not update editor values. Restore with cursorRestore().
void cursorTempPos(int x, int y)
{
    COORD pos = {x, y};
    SetConsoleCursorPosition(editor.hbuffer, pos);
}

// Restores cursor pos to where it was before call to cursorTempPos().
void cursorRestore()
{
    cursorSetPos(editor.col, editor.row);
}

// ---------------------- BUFFER ----------------------

// Write single character to current line.
void bufferWriteChar(char c)
{
    if (c < 32 || c > 126) // Reject non-ascii character
        return;

    Line *line = &editor.lines[editor.row];

    if (line->length >= line->cap - 1)
        // Extend line cap if exceeded
        bufferExtendLine(editor.row, line->cap + DEFAULT_LINE_LENGTH);

    if (editor.col < line->length)
    {
        // Move text when typing in the middle of a line
        char *pos = line->chars + editor.col;
        memmove(pos + 1, pos, line->length - editor.col);
    }

    line->chars[editor.col] = c;
    line->length++;
    editor.col++;
    bufferScroll(1, 0);
}

// Deletes the caharcter before the cursor position.
void bufferDeleteChar()
{
    Line *line = &editor.lines[editor.row];

    if (editor.col == 0)
    {
        if (editor.row == 0)
            return;

        // Delete line if cursor is at start
        int row = editor.row;
        cursorSetPos(editor.lines[editor.row - 1].length, editor.row - 1);
        bufferSplitLineUp(row);
        bufferDeleteLine(row);
        return;
    }

    if (editor.col <= line->length)
    {
        // Move chars when deleting in middle of line
        char *pos = line->chars + editor.col;
        memmove(pos - 1, pos, line->length - editor.col);
        line->chars[--line->length] = 0;
    }
    else
        line->chars[--line->length] = 0;

    editor.col--;
    bufferScroll(-1, 0);
}

// Creates an empty line at idx. Does not resize array.
void bufferCreateLine(int idx)
{
    Line line = {
        .chars = calloc(DEFAULT_LINE_LENGTH, sizeof(char)),
        .cap = DEFAULT_LINE_LENGTH,
        .row = idx,
        .length = 0,
        .idx = 0,
    };

    if (editor.indent > 0)
    {
        memset(line.chars, ' ', editor.indent);
        line.length = editor.indent;
    }

    check_pointer(line.chars, "bufferCreateLine");
    memcpy(&editor.lines[idx], &line, sizeof(Line));
    editor.numLines++;
}

// Realloc line character buffer
void bufferExtendLine(int row, int new_size)
{
    Line *line = &editor.lines[row];
    line->cap = new_size;
    line->chars = realloc(line->chars, line->cap);
    check_pointer(line->chars, "bufferExtendLine");
    memset(line->chars + line->length, 0, line->cap - line->length);
}

// Inserts new line at row. If row is -1 line is appended to end of file.
void bufferInsertLine(int row)
{
    row = row != -1 ? row : editor.numLines;

    if (editor.numLines >= editor.lineCap)
    {
        // Realloc editor line buffer array when full
        editor.lineCap += BUFFER_LINE_CAP;
        editor.lines = realloc(editor.lines, editor.lineCap * sizeof(Line));
        check_pointer(editor.lines, "bufferInsertLine");
    }

    if (row < editor.numLines)
    {
        // Move lines down when adding newline mid-file
        Line *pos = editor.lines + row;
        int count = editor.numLines - row;
        memmove(pos + 1, pos, count * sizeof(Line));
    }

    bufferCreateLine(row);
}

// Removes line at row.
void bufferDeleteLine(int row)
{
    free(editor.lines[row].chars);
    Line *pos = editor.lines + row + 1;

    if (row != editor.lineCap - 1)
    {
        int count = editor.numLines - row;
        memmove(pos - 1, pos, count * sizeof(Line));
        memset(editor.lines + editor.numLines, 0, sizeof(Line));
    }

    editor.numLines--;
}

// Moves characters behind cursor down to line below.
void bufferSplitLineDown(int row)
{
    if (row == editor.lineCap - 1)
    {
        // Debug
        logError("Failed to split down, not newline");
        return;
    }

    Line *from = &editor.lines[row];
    Line *to = &editor.lines[row + 1];
    int length = from->length - editor.col;

    if (to->cap <= length)
    {
        // Realloc line buffer so new text fits
        int l = DEFAULT_LINE_LENGTH;
        bufferExtendLine(row + 1, (length / l) * l + l);
    }

    // Copy characters and set right side of row to 0
    strcpy(to->chars + to->length, from->chars + editor.col);
    memset(from->chars + editor.col + to->length, 0, length);
    to->length += length;
    from->length -= length;
}

// Moves characters behind cursor to end of line above and deletes line.
void bufferSplitLineUp(int row)
{
    if (row == 0)
    {
        // Debug
        logError("Failed to split up, first row");
        return;
    }

    Line *from = &editor.lines[row];
    Line *to = &editor.lines[row - 1];

    if (from->length == 0)
        return;

    int length = from->length - editor.col + to->length;
    if (to->cap <= length)
    {
        // Realloc line buffer so new text fits
        int l = DEFAULT_LINE_LENGTH;
        bufferExtendLine(row - 1, (length / l) * l + l);
    }

    memcpy(to->chars + to->length, from->chars, from->length);
    to->length += from->length;
}

#define cursor_real_y (editor.row - editor.offy)
#define cursor_real_x (editor.col - editor.offx)

// Todo: fix horizontal scroll
// Todo: mouse scroll
// Scrolls text n spots up (negative), or down (positive).
void bufferScroll(int x, int y)
{
    // --- Vertical scroll ---

    // If cursor is scrolling up/down (within scroll threshold)
    if ((cursor_real_y > editor.textH - editor.scrollDy && y > 0) ||
        (cursor_real_y < editor.scrollDy && y < 0))
        editor.offy += y;

    // Do not let scroll go past end of file
    if (editor.offy + editor.textH > editor.numLines)
        editor.offy = editor.numLines - editor.textH;

    // Do not scroll past beginning or if page is not filled
    if (editor.offy < 0 || editor.numLines <= editor.textH)
        editor.offy = 0;

    // --- Horizontal scroll ---

    if (cursor_real_x > editor.textW - editor.scrollDx && x > 0)
        editor.offx += x;

    if (cursor_real_x == editor.textW - editor.scrollDx && x < 0)
        editor.offx += x;

    if (cursor_real_x < editor.textW - editor.scrollDx)
        editor.offx = 0;

    if (editor.offx < 0)
        editor.offx = 0;
}

// ---------------------- TYPING HELPERS ----------------------

const char begins[] = "({\"'[";
const char ends[] = ")}\"']";

// Matches braces, parens, strings etc with written char
void typingMatchParen(char c)
{
    Line line = editor.lines[editor.row];

    for (int i = 0; i < strlen(begins); i++)
    {
        if (c == begins[i])
        {
            bufferWriteChar(ends[i]);
            cursorMove(-1, 0);
            break;
        }

        if (c == ends[i] && line.chars[editor.col] == ends[i])
        {
            typingDeleteForward();
            break;
        }
    }
}

// When pressing enter after a paren, indent and move mathing paren to line below.
void typingBreakParen()
{
    Line line1 = editor.lines[editor.row];
    Line line2 = editor.lines[editor.row - 1];

    if (
        strchr(begins, line2.chars[line2.length - 1]) == NULL ||
        strchr(ends, line1.chars[editor.col]) == NULL)
        return;

    bufferWriteChar(' ');
    bufferWriteChar(' ');
    bufferWriteChar(' ');
    bufferWriteChar(' ');
    bufferInsertLine(editor.row + 1);
    bufferSplitLineDown(editor.row);
}

void typingDeleteForward()
{
    if (editor.col == editor.lines[editor.row].length)
    {
        if (editor.row == editor.numLines - 1)
            return;

        cursorHide();
        cursorSetPos(0, editor.row + 1);
    }
    else
    {
        cursorHide();
        cursorMove(1, 0);
    }

    bufferDeleteChar();
    cursorShow();
}

// ---------------------- RENDER ----------------------

void charbufAppend(CharBuffer *buf, char *src, int length)
{
    memcpy(buf->pos, src, length);
    buf->pos += length;
    buf->lineLength += length;
}

void charbufNextLine(CharBuffer *buf)
{
    int size = editor.width - buf->lineLength;
    for (int i = 0; i < size; i++)
        *(buf->pos++) = ' ';
    buf->lineLength = 0;
}

void charbufColor(CharBuffer *buf, char *col)
{
    int length = strlen(col);
    memcpy(buf->pos, col, length);
    buf->pos += length;
}

void charbufRender(CharBuffer *buf, int x, int y)
{
    cursorHide();
    cursorTempPos(x, y);
    screenBufferWrite(buf->buffer, buf->pos - buf->buffer);
    cursorRestore();
    cursorShow();
}

#define PAD_SIZE 64

#define color(col) charbufColor(&buf, col);
#define bg(col) color(BG(col));
#define fg(col) color(FG(col));

void renderBuffer()
{
    char padding[PAD_SIZE];
    memset(padding, (int)' ', PAD_SIZE);

    CharBuffer buf = {
        .buffer = editor.renderBuffer,
        .pos = editor.renderBuffer,
        .lineLength = 0,
    };

    // Draw lines
    for (int i = 0; i < editor.textH; i++)
    {
        int row = i + editor.offy;
        if (row >= editor.numLines)
            break;

        bg(COL_BG0);
        fg(COL_BG2);

        if (editor.row == row)
        {
            bg(COL_BG1);
            fg(COL_YELLOW);
        }

        // Line number
        char numbuf[12];
        sprintf(numbuf, " %4d ", row + 1);
        charbufAppend(&buf, numbuf, 6);

        fg(COL_FG0);

        // Line contents
        int lineLength = editor.lines[row].length - editor.offx;

        if (lineLength <= 0)
        {
            charbufNextLine(&buf);
            color(COL_RESET);
            continue;
        }

        if (editor.config.syntaxEnabled)
        {
            // Generate syntax highlighting for line and get new byte length
            int newLength;
            char *line = highlightLine(
                editor.lines[row].chars + editor.offx,
                min(lineLength, editor.textW),
                &newLength);

            charbufAppend(&buf, line, newLength);

            // Subtract added highlight strings from line length as they are 0-width
            int diff = newLength - lineLength;
            buf.lineLength -= diff;
        }
        else
            charbufAppend(&buf, editor.lines[row].chars + editor.offx, lineLength);

        // Add padding at end for horizontal scroll
        int off = editor.textW - lineLength;
        if (editor.offx > 0 && off > 0)
            charbufAppend(&buf, padding, off);

        charbufNextLine(&buf);
        color(COL_RESET);
    }

    bg(COL_BG0);
    fg(COL_BG2);

    // Draw squiggles for non-filled lines
    if (editor.numLines < editor.textH)
        for (int i = 0; i < editor.textH - editor.numLines; i++)
        {
            charbufAppend(&buf, "~", 1);
            charbufNextLine(&buf);
        }

    color(COL_RESET);
    charbufRender(&buf, 0, 0);
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

// Updates status bar values and calls statusBarRender.
void statusBarUpdate(char *filename)
{
    // editor.info = (Info){};
    strcpy(editor.info.filename, filename);
    statusBarRender();
}

void statusBarRender()
{
    CharBuffer buf = {
        .buffer = editor.renderBuffer,
        .pos = editor.renderBuffer,
        .lineLength = 0,
    };

    bg(COL_FG0);
    fg(COL_BG0);

    char *filename = editor.info.filename;
    charbufAppend(&buf, filename, strlen(filename));

    bg(COL_BG1);
    fg(COL_FG0);
    charbufNextLine(&buf);

    bg(COL_BG0);
    charbufNextLine(&buf);

    color(COL_RESET);
    charbufRender(&buf, 0, editor.height - 2);
}