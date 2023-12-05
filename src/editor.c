#include "editor.h"
#include "util.h"

// ---------------------- GLOBAL STATE ----------------------

typedef struct linebuf
{
    int idx; // Row index in file, not buffer
    int row; // Relative row in buffer, not file

    int cap;     // Capacity of line
    int length;  // Length of line
    char *chars; // Characters in line
} linebuf;

struct editorGlobals
{
    HANDLE hstdin;  // Handle for standard input
    HANDLE hbuffer; // Handle to new screen buffer

    int width, height; // Size of terminal window
    int textW, textH;  // Size of text editing area
    int padV, padH;    // Vertical and horizontal padding

    int row, col;      // Current row and col of cursor in buffer
    int offx, offy;    // x, y offset from left/top

    int scrollDistX, scrollDistY; // Minimum distance from top/bottom or left/right before scrolling

    int numLines, lineCap; // Count and capacity of lines in array
    linebuf *lines;        // Array of lines in buffer
    char *renderBuffer;    // Written to and printed on render
} editor;

#define editorBufferSize ((editor.width+1) * editor.height)

// ---------------------- EDITOR ----------------------

// Populates editor global struct and creates empty file buffer. Exits on error.
void editorInit()
{
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

    if (editorTerminalGetSize() == RETURN_ERROR)
    {
        logError("editorInit() - Failed to get buffer size");
        ExitProcess(EXIT_FAILURE);
    }

    editor.padH = 0;
    editor.padV = 2; // Status line

    editor.textW = editor.width - editor.padH;
    editor.textH = editor.height - editor.padV;

    logNumber("Terminal width: ", editor.width);
    logNumber("Terminal height: ", editor.height);
}

// Free, clean, and exit
void editorExit()
{
    for (int i = 0; i < editor.numLines; i++)
        free(editor.lines[i].chars);

    free(editor.lines);
    free(editor.renderBuffer);
    CloseHandle(editor.hbuffer);
    ExitProcess(EXIT_SUCCESS);
}

// Update editor size values. Returns -1 on error.
int editorTerminalGetSize()
{
    CONSOLE_SCREEN_BUFFER_INFO cinfo;
    if (!GetConsoleScreenBufferInfo(editor.hbuffer, &cinfo))
        return_error("editorTerminalGetSize() - Failed to get buffer info");

    editor.width = (int)cinfo.srWindow.Right;
    editor.height = (int)(cinfo.srWindow.Bottom + 1);

    return RETURN_SUCCESS;
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
        return_error("editorHandleInput() - Failed to read input");

    if (record.EventType == KEY_EVENT)
    {
        if (record.Event.KeyEvent.bKeyDown)
        {
            KEY_EVENT_RECORD event = record.Event.KeyEvent;
            WORD keyCode = event.wVirtualKeyCode;
            char inputChar = event.uChar.AsciiChar;

            switch (keyCode)
            {
            case K_ESCAPE:
                editorExit();

            case K_BACKSPACE:
                bufferDeleteChar();
                break;

            // Todo: wtf is this shit
            case K_DELETE:
            {
                if (editor.col == editor.lines[editor.row].length)
                {
                    if (editor.row == editor.numLines - 1)
                        break;

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
                break;
            }

            case K_ENTER:
                bufferInsertLine(editor.row + 1);
                bufferSplitLineDown(editor.row);
                cursorSetPos(0, editor.row + 1);
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

            case K_PAGEUP:
                bufferScroll(-1);
                cursorMove(0, -1);
                break;

            case K_PAGEDOWN:
                bufferScroll(1);
                cursorMove(0, 1);
                break;

            default:
                bufferWriteChar(inputChar);
            }
        }
    }

    renderBuffer();
    return RETURN_SUCCESS;
}

// Loads file into buffer. Filepath must either be an absolute path
// or name of a file in the same directory as wim.
int editorLoadFile(const char *filepath)
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

    // Read file line by line
    char *newline;
    char *ptr = buffer;
    int row = 0;

    // Todo: read last line also if not ending with newline char
    while ((newline = strstr(ptr, "\n")) != NULL)
    {
        // Realloc buffer line array if full
        if (editor.numLines >= editor.lineCap)
        {
            // Realloc editor line buffer array when full
            editor.lineCap += BUFFER_LINE_CAP;
            editor.lines = realloc(editor.lines, editor.lineCap * sizeof(linebuf));
            check_pointer(editor.lines, "bufferInsertLine");
        }

        // Get distance from current pos in buffer and found newline
        // Then strncpy the line into the line char buffer
        int length = newline - ptr;

        linebuf line = {
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
        strncpy(chars, ptr, length - 1);

        // Fill out line values and copy line to line array
        line.cap = cap;
        line.chars = chars;
        memcpy(&editor.lines[row], &line, sizeof(linebuf));

        // Increment number of line, position in buffer, and row
        editor.numLines = row + 1;
        ptr += length + 1;
        row++;
    }

    renderBuffer();
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
    bufferScroll(y - editor.row); // Scroll by cursor offset

    editor.col = x;
    editor.row = y;

    // Cursor out of bounds
    if (editor.col < 0)
        editor.col = 0;
    if (editor.col > editor.lines[editor.row].length)
        editor.col = editor.lines[editor.row].length;
    if (editor.row < 0)
        editor.row = 0;
    if (editor.row > editor.numLines - 1)
        editor.row = editor.numLines - 1;

    COORD pos = {editor.col + editor.offx, editor.row - editor.offy};
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

// Creates an empty file buffer.
void bufferCreate()
{
    editor.offx = 4;
    editor.offy = 0;
    editor.scrollDistX = 0;
    editor.scrollDistY = 5;
    editor.numLines = 0;
    editor.lineCap = BUFFER_LINE_CAP;
    editor.lines = calloc(editor.lineCap, sizeof(linebuf));
    editor.renderBuffer = malloc(2 * editorBufferSize);
    check_pointer(editor.lines, "bufferCreate");
    check_pointer(editor.renderBuffer, "bufferCreate");
    bufferCreateLine(0);
    renderBuffer();
}

// Write single character to current line.
void bufferWriteChar(char c)
{
    if (c < 32 || c > 126) // Reject non-ascii character
        return;

    if (editor.col >= editor.textW - 1)
        // Todo: implement horizontal text scrolling
        return;

    linebuf *line = &editor.lines[editor.row];

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
}

// Deletes the caharcter before the cursor position.
void bufferDeleteChar()
{
    linebuf *line = &editor.lines[editor.row];

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
}

// Creates an empty line at idx. Does not resize array.
void bufferCreateLine(int idx)
{
    linebuf line = {
        .chars = calloc(DEFAULT_LINE_LENGTH, sizeof(char)),
        .cap = DEFAULT_LINE_LENGTH,
        .row = idx,
        .length = 0,
        .idx = 0,
    };

    check_pointer(line.chars, "bufferCreateLine");
    memcpy(&editor.lines[idx], &line, sizeof(linebuf));
    editor.numLines++;
}

// Realloc line character buffer
void bufferExtendLine(int row, int new_size)
{
    linebuf *line = &editor.lines[row];
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
        editor.lines = realloc(editor.lines, editor.lineCap * sizeof(linebuf));
        check_pointer(editor.lines, "bufferInsertLine");
    }

    if (row < editor.numLines)
    {
        // Move lines down when adding newline mid-file
        linebuf *pos = editor.lines + row;
        int count = editor.numLines - row;
        memmove(pos + 1, pos, count * sizeof(linebuf));
    }

    bufferCreateLine(row);
}

// Removes line at row.
void bufferDeleteLine(int row)
{
    free(editor.lines[row].chars);
    linebuf *pos = editor.lines + row + 1;

    if (row != editor.lineCap - 1)
    {
        int count = editor.numLines - row;
        memmove(pos - 1, pos, count * sizeof(linebuf));
        memset(editor.lines + editor.numLines, 0, sizeof(linebuf));
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

    linebuf *from = &editor.lines[row];
    linebuf *to = &editor.lines[row + 1];
    int length = from->length - editor.col;

    if (to->cap < length)
    {
        // Realloc line buffer so new text fits
        int l = DEFAULT_LINE_LENGTH;
        bufferExtendLine(row + 1, (length / l) * l + l);
    }

    // Copy characters and set right side of row to 0
    strcpy(to->chars, from->chars + editor.col);
    memset(from->chars + editor.col, 0, length);
    to->length = length;
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

    linebuf *from = &editor.lines[row];
    linebuf *to = &editor.lines[row - 1];

    if (from->length == 0)
        return;

    int length = from->length - editor.col + to->length;
    if (to->cap < length)
    {
        // Realloc line buffer so new text fits
        int l = DEFAULT_LINE_LENGTH;
        bufferExtendLine(row - 1, (length / l) * l + l);
    }

    memcpy(to->chars + to->length, from->chars, from->length);
    to->length += from->length;
}

// Scrolls text n spots up (negative), or down (positive).
void bufferScroll(int n)
{
    // If cursor is scrolling up/down (within scroll threshold)
    if ((cursor_real_y > editor.textH - editor.scrollDistY && n > 0) ||
        (cursor_real_y < editor.scrollDistY && n < 0))
        editor.offy += n;

    // Do not let scroll go past end of file
    if (editor.offy + editor.textH > editor.numLines)
        editor.offy = editor.numLines - editor.textH;

    // Do not scroll past beginning or if page is not filled
    if (editor.offy < 0 || editor.numLines <= editor.textH)
        editor.offy = 0;
}

// ---------------------- RENDER ----------------------

typedef struct charbuf
{
    char *buffer;
    char *pos;
    int lineLength;
} charbuf;

void charbufAppend(charbuf *buf, char *src, int length)
{
    memcpy(buf->pos, src, length);
    buf->pos += length;
    buf->lineLength += length;
}

void charbufNextLine(charbuf *buf)
{
    int size = editor.width - buf->lineLength + 1;
    for (int i = 0; i < size; i++)
        *(buf->pos++) = ' ';
    buf->lineLength = 0;
}

void charbufColor(charbuf *buf, char *col)
{
    int length = strlen(col);
    memcpy(buf->pos, col, length);
    buf->pos += length;
}

void charbufRender(charbuf *buf, int x, int y)
{
    cursorHide();
    cursorTempPos(x, y);
    screenBufferWrite(buf->buffer, buf->pos - buf->buffer);
    cursorRestore();
    cursorShow();
}

void renderBuffer()
{
    charbuf buf = {
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

        // Line number
        char numbuf[12] = "    ";
        int a = sprintf(numbuf, "%d", row + 1);
        numbuf[a] = ' ';
        charbufAppend(&buf, numbuf, 4);

        // Line contents
        int lineLength = editor.lines[row].length;
        charbufAppend(&buf, editor.lines[row].chars, lineLength);
        charbufNextLine(&buf);
    }

    // Draw squiggles for non-filled lines
    if (editor.numLines < editor.textH)
        for (int i = 0; i < editor.textH - editor.numLines; i++)
            charbufAppend(&buf, "~     \n", 7);

    charbufRender(&buf, 0, 0);
}

// Todo: Updates status bar info with given arguments, if left NULL, the previous stays.
void renderSatusBar(char *filename)
{
    charbuf buf = {
        .buffer = editor.renderBuffer,
        .pos = editor.renderBuffer,
        .lineLength = 0,
    };

    // White bar
    charbufColor(&buf, COL_BG_WHITE);
    charbufAppend(&buf, filename, strlen(filename));
    charbufNextLine(&buf);
    charbufColor(&buf, COL_RESET);

    charbufRender(&buf, 0, editor.height-2);
}