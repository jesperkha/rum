// All things related to writing, deleting, and modifying things in the current buffer.

#include "rum.h"

extern Editor editor;
extern Colors colors;
extern Config config;

// Reallocs lines char array to new size.
static void bufferExtendLine(Buffer *b, int row, int new_size)
{
    if (row >= curBuffer->numLines)
        Panicf("row %d out of bounds", row);

    Line *line = &b->lines[row];
    line->cap = new_size;
    line->chars = MemRealloc(line->chars, line->cap);
    AssertNotNull(line->chars);
    // memset(line->chars + line->length, 0, line->cap - line->length);
}

Buffer *BufferNew()
{
    Buffer *b = MemZeroAlloc(sizeof(Buffer));
    b->lineCap = BUFFER_DEFAULT_LINE_CAP;
    b->lines = MemZeroAlloc(b->lineCap * sizeof(Line));
    AssertNotNull(b->lines);
    b->undos = UndoNewList();

    b->padX = 6; // Line numbers
    b->padY = 0;

    b->cursor = (Cursor){
        .scrollDx = 5,
        .scrollDy = 5,
        .visible = true,
    };

    BufferInsertLine(b, 0);
    b->searchLen = 0;
    b->offX = 0;

    b->dirty = false;
    b->readOnly = false;
    b->isDir = false;
    b->useTabs = false;
    b->showCurrentLineMark = true;
    return b;
}

void BufferFree(Buffer *b)
{
    for (int i = 0; i < b->numLines; i++)
        MemFree(b->lines[i].chars);

    if (b->isDir)
        StrArrayFree(&b->exPaths);

    MemFree(b->lines);
    MemFree(b);
}

// Writes characters to buffer at row/col.
void BufferWriteEx(Buffer *b, int row, int col, char *source, int length)
{
    Line *line = &b->lines[row];

    if (line->length + length >= line->cap)
    {
        // Allocate enough memory for the total string
        int l = LINE_DEFAULT_LENGTH;
        int requiredSpace = (length / l + 1) * l;
        bufferExtendLine(b, row, line->cap + requiredSpace);
    }

    if (col < line->length)
    {
        // Move text when typing in the middle of a line
        char *pos = line->chars + col;
        memmove(pos + length, pos, line->length - col);
    }

    memcpy(line->chars + col, source, length);
    line->length += length;
    line->isMarked = false;
    b->dirty = true;
}

// Writes characters to buffer at cursor position.
void BufferWrite(Buffer *b, char *source, int length)
{
    BufferWriteEx(b, b->cursor.row, b->cursor.col, source, length);
}

// Writes to buffer at row/col. Replaces any characters that are already there.
void BufferOverWriteEx(Buffer *b, int row, int col, char *source, int length)
{
    Line *line = &b->lines[row];

    if (line->length + length >= line->cap)
    {
        // Allocate enough memory for the total string
        int l = LINE_DEFAULT_LENGTH;
        int requiredSpace = (length / l + 1) * l;
        bufferExtendLine(b, row, line->cap + requiredSpace);
    }

    memcpy(line->chars + col, source, length);
    line->length = max(line->length, col + length);
    line->isMarked = false;
    b->dirty = true;
}

// Writes to buffer at current row/col. Replaces any characters that are already there.
void BufferOverWrite(Buffer *b, char *source, int length)
{
    BufferOverWriteEx(b, b->cursor.row, b->cursor.col, source, length);
}

// Deletes backwards from col at row. Stops at empty line, does not remove newline.
void BufferDeleteEx(Buffer *b, int row, int col, int count)
{
    if (col == 0)
        return;

    Line *line = &b->lines[row];
    count = min(count, col); // Dont delete past 0

    if (col <= line->length)
    {
        // Move chars when deleting in middle of line
        char *pos = line->chars + col;
        memmove(pos - count, pos, line->length - col);
    }

    memset(line->chars + line->length, 0, line->cap - line->length);
    line->length -= count;
    line->isMarked = false;
    b->dirty = true;
}

// Deletes backwards from cursor position. Stops at empty line, does not remove newline.
void BufferDelete(Buffer *b, int count)
{
    BufferDeleteEx(b, b->cursor.row, b->cursor.col, count);
}

// Returns number of spaces before the cursor
int BufferGetPrefixedSpaces(Buffer *b)
{
    Line *line = &b->lines[b->cursor.row];
    int prefixedSpaces = 0;

    for (int i = b->cursor.col - 1; i >= 0; i--)
    {
        if (line->chars[i] != ' ')
            break;
        prefixedSpaces++;
    }

    return prefixedSpaces;
}

Line *BufferInsertLine(Buffer *b, int row)
{
    return BufferInsertLineEx(b, row, NULL, 0);
}

Line *BufferInsertLineEx(Buffer *b, int row, char *text, int textLen)
{
    row = row != -1 ? row : b->numLines;

    if (b->numLines >= b->lineCap)
    {
        // Realloc editor line buffer array when full
        b->lineCap += BUFFER_DEFAULT_LINE_CAP;
        b->lines = MemRealloc(b->lines, b->lineCap * sizeof(Line));
        AssertNotNull(b->lines);
    }

    if (row < b->numLines)
    {
        // Move lines down when adding newline mid-file
        Line *pos = b->lines + row;
        int count = b->numLines - row;
        memmove(pos + 1, pos, count * sizeof(Line));
    }

    char *chars = NULL;
    int cap = LINE_DEFAULT_LENGTH;

    if (text != NULL)
    {
        // Copy over text and set correct line length/cap
        if (textLen >= cap)
        {
            int l = LINE_DEFAULT_LENGTH;
            cap = (textLen / l) * l + l;
        }

        chars = MemZeroAlloc(cap * sizeof(char));
        AssertNotNull(chars);
        strncat(chars, text, textLen);
    }
    else
    {
        // No text was passed
        chars = MemZeroAlloc(LINE_DEFAULT_LENGTH * sizeof(char));
        AssertNotNull(chars);
    }

    Line line = {
        .chars = chars,
        .cap = cap,
        .row = row,
        .length = strlen(chars),
        .exPathId = 0,
        .isPath = false,
    };

    memcpy(&b->lines[row], &line, sizeof(Line));
    b->numLines++;
    b->dirty = true;

    return &b->lines[row];
}

// Deletes line at row and move all lines below upwards.
void BufferDeleteLine(Buffer *b, int row)
{
    // Swap to last row if -1
    row = row != -1 ? row : b->numLines - 1;

    if (row > b->numLines - 1)
        Panicf("row %d out of bounds", row);

    Line *line = &b->lines[row];

    if (row == 0 && b->numLines == 1)
    {
        memset(line->chars, 0, line->cap);
        line->length = 0;
        return;
    }

    MemFree(line->chars);
    Line *pos = b->lines + row + 1;

    if (row != b->lineCap - 1)
    {
        int count = b->numLines - row;
        memmove(pos - 1, pos, count * sizeof(Line));
        memset(b->lines + b->numLines, 0, sizeof(Line));
    }

    b->numLines--;
    b->dirty = true;
}

// Copies and removes all characters behind the cursor position,
// then pastes them at the end of the line below.
void BufferMoveTextDownEx(Buffer *b, int row, int col)
{
    Line *from = &b->lines[row];
    Line *to = &b->lines[row + 1];
    int length = from->length - col;

    if (to->cap <= length + to->length)
    {
        // Realloc line buffer so new text fits
        int l = LINE_DEFAULT_LENGTH;
        bufferExtendLine(b, row + 1, (length / l) * l + l);
    }

    // Copy characters and set right side of row to 0
    strcpy(to->chars + to->length, from->chars + col);
    memset(from->chars + col + to->length, 0, length);
    to->length += length;
    from->length -= length;
    b->dirty = true;
    from->isMarked = to->isMarked = false;
}

// Copies and removes all characters behind the cursor position,
// then pastes them at the end of the line below.
void BufferMoveTextDown(Buffer *b)
{
    BufferMoveTextDownEx(b, b->cursor.row, b->cursor.col);
}

// Moves line content from row to end of line above. Returns length of line above.
int BufferMoveTextUpEx(Buffer *b, int row, int col)
{
    Line *from = &b->lines[row];
    Line *to = &b->lines[row - 1];
    int toLength = to->length;

    if (from->length == 0)
        return toLength;

    int length = from->length - col + to->length;
    if (to->cap <= length)
    {
        // Realloc line buffer so new text fits
        int l = LINE_DEFAULT_LENGTH;
        bufferExtendLine(b, row - 1, (length / l) * l + l);
    }

    memcpy(to->chars + to->length, from->chars, from->length);
    to->length += from->length;
    b->dirty = true;
    from->isMarked = to->isMarked = false;
    return toLength;
}

// Moves line content from row to end of line above. Returns length of line above.
int BufferMoveTextUp(Buffer *b)
{
    return BufferMoveTextUpEx(b, b->cursor.row, b->cursor.col);
}

void BufferScroll(Buffer *b)
{
    // Dont scroll when the whole file is in view
    if (b->numLines <= b->textH)
    {
        b->cursor.offy = 0;
        return;
    }

    int screen_y = (b->cursor.row - b->cursor.offy);

    // Scrolling up
    if (screen_y < b->cursor.scrollDy)
        b->cursor.offy = max(b->cursor.row - b->cursor.scrollDy, 0);

    // Scrolling down
    else if (screen_y > b->textH - b->cursor.scrollDy)
        b->cursor.offy = min(
            b->cursor.row - b->textH + b->cursor.scrollDy,
            b->numLines - b->textH + b->cursor.scrollDy);

    Assert(b->cursor.offy >= 0);
}

static void renderLine(Buffer *b, CharBuf *cb, int idx, int maxWidth)
{
    // Hide text when ui is open to not clutter view
    if (editor.uiOpen && curBuffer->id == b->id)
    {
        CbColor(cb, colors.bg0, colors.fg0);
        CbAppend(cb, editor.padBuffer, maxWidth);
        return;
    }

    int textW = maxWidth - b->padX;
    int row = idx + b->cursor.offy;

    if (row < b->numLines)
    {
        Line line = b->lines[row];

        // Line background color
        bool isCurrentLine = b->id == editor.activeBuffer && b->cursor.row == row && !b->showHighlight && b->showCurrentLineMark;
        isCurrentLine ? CbColor(cb, colors.bg1, colors.fg0) : CbColor(cb, colors.bg0, colors.bg2);

        // Line numbers
        {
            char numbuf[12];
            sprintf(numbuf, " %4d ", (short)(row + 1));
            CbAppend(cb, numbuf, b->padX);
        }

        // Line contents
        CbFg(cb, colors.fg0);
        b->cursor.offx = max(b->cursor.col - textW + b->cursor.scrollDx, 0);

        int lineLength = line.length - b->cursor.offx;
        int renderLength = clamp(0, editor.width, min(lineLength, textW));
        char *lineBegin = line.chars + b->cursor.offx;

        // Add a single blank so highlighting shows up on empty lines too
        if (lineLength == 0)
        {
            renderLength = 1;
            lineBegin = editor.padBuffer;
        }

        // Add color and highlights to line
        {
            HlLine finalLine = {
                .length = renderLength,
                .rawLength = renderLength,
                .line = lineBegin,
                .row = row,
                .isCurrentLine = isCurrentLine,
            };

            if (config.syntaxEnabled)
                finalLine = ColorLine(b, finalLine);

            if (b->showHighlight)
                finalLine = HighlightLine(b, finalLine);

            if (b->showMarkedLines && line.isMarked)
                finalLine = MarkLine(finalLine, line.hlStart, line.hlEnd);

            CbAppend(cb, finalLine.line, finalLine.length);
        }

        // Padding after
        if (renderLength < textW)
        {
            CbAppend(cb, editor.padBuffer, textW - renderLength);
        }
    }
    else
    {
        CbColor(cb, colors.bg0, colors.bg2);
        CbAppend(cb, "~", 1);
        CbAppend(cb, editor.padBuffer, maxWidth - 1);
    }
}

static void renderStatusLine(Buffer *b, CharBuf *cb, int maxWidth)
{
    cb->lineLength = 0; // Reset to get length of status info

    if (b->id == editor.activeBuffer)
    {
        CbColor(cb, colors.fg0, colors.bg0);
        if (editor.mode == MODE_EDIT)
            CbAppend(cb, "EDIT", 4);
        else if (editor.mode == MODE_INSERT)
            CbAppend(cb, "INSERT", 6);
        else if (editor.mode == MODE_VISUAL)
            CbAppend(cb, "VISUAL", 6);
        else if (editor.mode == MODE_VISUAL_LINE)
            CbAppend(cb, "VISUAL-LINE", 11);
        else if (editor.mode == MODE_EXPLORE)
            CbAppend(cb, "EXPLORER", 8);
        else
            Panic("Unhandled mode for statusline");
        CbColor(cb, colors.bg1, colors.fg0);
        CbAppend(cb, " ", 1);
    }
    else
        CbColor(cb, colors.bg1, colors.fg0);

    // Read-only flag
    if (b->readOnly)
    {
        CbAppend(cb, b->filepath, strlen(b->filepath));
        CbColor(cb, colors.bg1, colors.keyword);
        CbAppend(cb, " (READ-ONLY)", 12);
    }

    // Filename
    else if (b->isFile)
    {
        CbAppend(cb, b->filepath, strlen(b->filepath));
        if (b->dirty && b->isFile && !b->readOnly)
            CbAppend(cb, "*", 1);
    }
    else
        CbAppend(cb, "[empty]", 7);

    // File size and num lines
    if (!b->isDir)
    {
        CbColor(cb, colors.bg1, colors.fg0);
        CbAppend(cb, " | ", 3);

        char fInfo[256];
        int infoLen = 0;

        infoLen = sprintf(fInfo, "lines %d  ", b->numLines);
        CbAppend(cb, fInfo, infoLen);

        infoLen = sprintf(fInfo, b->useTabs ? "tabs  " : "spaces %d  ", config.tabSize);
        CbAppend(cb, fInfo, infoLen);

        CbAppend(cb, config.useCRLF ? "CRLF" : "LF  ", 4); // last
    }

    CbAppend(cb, editor.padBuffer, maxWidth - cb->lineLength);
}

void BufferRenderFull(Buffer *b)
{
    CharBuf cb = CbNew(editor.renderBuffer);

    int h = editor.height - 2;
    b->textH = h - b->padY;

    b->width = editor.width;
    b->height = editor.height - 2;
    b->offX = 0;

    for (int i = 0; i < b->textH; i++)
        renderLine(b, &cb, i, editor.width);

    renderStatusLine(b, &cb, editor.width);
    CbRender(&cb, 0, 0);
}

void BufferRenderSplit(Buffer *a, Buffer *b)
{
    CharBuf cb = CbNew(editor.renderBuffer);

    int h = editor.height - 2;
    int textH = h - a->padY;
    a->textH = h - a->padY;
    b->textH = h - b->padY;

    int gutterW = 3;
    int leftW = editor.width / 2 - 1;
    int rightW = editor.width / 2 - 2;
    if (editor.width % 2 != 0)
        rightW++;

    a->offX = 0;
    b->offX = editor.width - rightW;

    a->width = leftW;
    b->width = rightW;
    a->height = b->height = h;

    char gutter[] = {' ', (char)179, ' ', ' ', 0};

    for (int i = 0; i < textH; i++)
    {
        renderLine(a, &cb, i, leftW);
        CbColor(&cb, colors.bg0, colors.bg1);
        CbAppend(&cb, gutter, gutterW);
        renderLine(b, &cb, i, rightW);
    }

    renderStatusLine(a, &cb, leftW);
    CbColor(&cb, colors.bg0, colors.bg1);
    CbAppend(&cb, gutter, gutterW);
    renderStatusLine(b, &cb, rightW);

    CbRender(&cb, 0, 0);
}

static String expandTabs(String s)
{
    int newLength = s.length;
    strncpy(editor.renderBuffer, s.s, s.length);

    for (int i = 0; i < newLength; i++)
    {
        char c = editor.renderBuffer[i];
        if (c == '\t')
        {
            int tab = config.tabSize;
            char *pos = editor.renderBuffer + i;

            memmove(pos + tab - 1, pos, newLength - i);
            memset(pos, ' ', tab);
            newLength += tab - 1;
        }
    }

    editor.renderBuffer[newLength] = 0;
    return STRING(editor.renderBuffer, newLength);
}

static String contractTabs(String s)
{
    int newLength = s.length;
    strncpy(editor.renderBuffer, s.s, s.length);

    for (int i = 0; i < newLength; i++)
    {
        if (i + config.tabSize > newLength)
            continue;

        char *pos = editor.renderBuffer + i;
        int tab = config.tabSize;

        if (!strncmp(editor.padBuffer, pos, tab))
        {
            memmove(pos, pos + tab - 1, newLength - i);
            *pos = '\t';
            newLength -= tab - 1;
        }
    }

    editor.renderBuffer[newLength] = 0;
    return STRING(editor.renderBuffer, newLength);
}

// Loads file contents into a new Buffer and returns it.
Buffer *BufferLoadFile(char *filepath, char *buf, int size)
{
    Logf("Loading file %s, size %d bytes", filepath, size);

    Buffer *b = BufferNew();
    BufferSetFilename(b, filepath);

    char *newline;
    char *ptr = buf;
    int row = 0;

    while ((newline = strstr(ptr, "\n")) != NULL)
    {
        // Get distance from current pos in buffer and found newline
        // Then strncpy the line into the line char buffer
        int length = newline - ptr;

        String line = expandTabs(STRING(ptr, length));
        if (line.length != length)
            b->useTabs = true;

        if (line.length > 0 && *(newline - 1) == '\r')
            BufferInsertLineEx(b, row, line.s, line.length - 1);
        else
            BufferInsertLineEx(b, row, line.s, line.length);

        ptr += length + 1;
        row++;
    }

    // Write last line of file
    String line = expandTabs(STRING(ptr, (size - (ptr - buf))));
    BufferInsertLineEx(b, row, line.s, line.length);
    BufferDeleteLine(b, -1); // Remove line added at buffer create
    b->dirty = false;
    return b;
}

// Saves buffer contents to file. Returns true on success.
bool BufferSaveFile(Buffer *b)
{
    if (b->readOnly)
        return false;

    // Give file name before saving if blank
    if (!b->isFile)
    {
        UiResult res = UiGetTextInput("Filename: ", 64);
        if (res.status != UI_OK || res.length == 0)
        {
            UiFreeResult(res);
            return false;
        }

        BufferSetFilename(b, res.buffer);
        UiFreeResult(res);
    }

    bool CRLF = config.useCRLF;

    // Accumulate size of buffer by line length
    int size = 0;
    int newlineSize = CRLF ? 2 : 1;

    for (int i = 0; i < b->numLines; i++)
        size += b->lines[i].length + newlineSize;

    // Write to buffer, add newline for each line
    char buf[size];
    char *ptr = buf;
    for (int i = 0; i < b->numLines; i++)
    {
        Line line = b->lines[i];

        String linestr = STRING(line.chars, line.length);
        if (b->useTabs)
            linestr = contractTabs(linestr);

        memcpy(ptr, linestr.s, linestr.length);
        ptr += linestr.length;
        if (CRLF)
            *(ptr++) = '\r'; // CR
        *(ptr++) = '\n';     // LF
    }

    int newSize = ptr - buf;
    IoWriteFile(b->filepath, buf, newSize - newlineSize);
    b->dirty = false;
    return true;
}

void BufferCenterView(Buffer *b)
{
    b->cursor.offy = max(min(b->cursor.row - b->textH / 2, b->numLines - b->textH), 0);
}

void BufferOrderHighlightPoints(Buffer *b, CursorPos *from, CursorPos *to)
{
    bool n = b->hlA.row < b->hlB.row || (b->hlA.row == b->hlB.row && b->hlA.col < b->hlB.col);
    *from = n ? b->hlA : b->hlB;
    *to = n ? b->hlB : b->hlA;

    to->col++; // Hack to make marker always at least 1 char wide
}

// Returns the text hihglighted in visual mode
char *BufferGetMarkedText(Buffer *b)
{
    CharBuf cb = CbNew(editor.renderBuffer);

    CursorPos from, to;
    BufferOrderHighlightPoints(b, &from, &to);

    for (int i = from.row; i <= to.row; i++)
    {
        Line line = b->lines[i];
        if (line.length > 0)
        {
            int start = from.row == i ? from.col : 0;
            int end = to.row == i ? to.col : line.length;
            CbAppend(&cb, line.chars + start, end - start);
        }

        if (config.useCRLF)
            CbAppend(&cb, "\r\n", 2);
        else
            CbAppend(&cb, "\n", 1);
    }

    cb.buffer[cb.lineLength - 1] = 0; // Make c-string and remove last newline
    return cb.buffer;
}

char *BufferGetLinePath(Buffer *b, Line *line)
{
    Assert(line->isPath);
    return StrArrayGet(&b->exPaths, line->exPathId);
}

void BufferMarkLine(Buffer *b, int row, int col, int length)
{
    Line *line = &b->lines[row];
    line->hlStart = col;
    line->hlEnd = col + length;
    line->isMarked = true;
}

void BufferUnmarkAll(Buffer *b)
{
    for (int i = 0; i < b->numLines; i++)
        b->lines[i].isMarked = false;
}

void BufferSetSearchWord(Buffer *b, char *search, int length)
{
    if (search == NULL)
        b->searchLen = 0;
    strncpy(b->search, search, length);
    b->searchLen = length;
}

void BufferSetFilename(Buffer *b, char *filepath)
{
    strncpy(b->filepath, filepath, MAX_PATH);
    b->isFile = true;

    char extension[FILE_EXTENSION_SIZE];
    StrFileExtension(extension, filepath);
    BufferSetFileType(b, extension);
}

bool BufferSetFileType(Buffer *b, const char *extension)
{
#define is(ex) !strcmp(extension, ex)
    FileType t = FT_UNKNOWN;

    if (is("c") || is("h"))
        t = FT_C;
    else if (is("py"))
        t = FT_PYTHON;
    else if (is("json"))
        t = FT_JSON;

    b->fileType = t;
    return t != FT_UNKNOWN;
}