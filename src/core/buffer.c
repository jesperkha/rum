#include "wim.h"

static void bufferExtendLine(int row, int new_size);

// Writes characters to buffer at cursor pos. Does not filter out non-writable characters.
void BufferWrite(char *source, int length)
{
    Editor *e = editorGetHandle();
    Line *line = &e->lines[e->row];

    if (line->length + length >= line->cap)
    {
        // Allocate enough memory for the total string
        int l = DEFAULT_LINE_LENGTH;
        int requiredSpace = (length / l + 1) * l;
        bufferExtendLine(e->row, line->cap + requiredSpace);
    }

    if (e->col < line->length)
    {
        // Move text when typing in the middle of a line
        char *pos = line->chars + e->col;
        memmove(pos + length, pos, line->length - e->col);
    }

    memcpy(line->chars + e->col, source, length);
    line->length += length;
    e->col += length;
    e->info.dirty = true;
}

// Deletes the caharcter before the cursor position. Deletes line if cursor is at beginning.
void BufferDeleteChar()
{
    Editor *e = editorGetHandle();
    Line *line = &e->lines[e->row];

    if (e->col == 0)
    {
        if (e->row == 0)
            return;

        // Delete line if cursor is at start
        int row = e->row;
        int length = e->lines[e->row - 1].length;

        cursorSetPos(length, e->row - 1, false);
        BufferSplitLineUp(row);
        BufferDeleteLine(row);
        cursorSetPos(length, e->row, false);
        return;
    }

    // Delete tabs
    int prefixedSpaces = 0;
    for (int i = e->col-1; i >= 0; i--)
    {
        if (line->chars[i] != ' ')
            break;

        prefixedSpaces++;
    }

    int deleteCount = 1;
    int tabSize = e->config.tabSize;
    if (prefixedSpaces > 0 && prefixedSpaces % tabSize == 0)
        deleteCount = tabSize;

    if (e->col <= line->length)
    {
        // Move chars when deleting in middle of line
        char *pos = line->chars + e->col;
        memmove(pos - deleteCount, pos, line->length - e->col);
    }

    memset(line->chars + line->length, 0, line->cap - line->length);
    line->length -= deleteCount;
    e->col -= deleteCount;
    e->info.dirty = true;
}

// Inserts new line at row. If row is -1 line is appended to end of file.
void BufferInsertLine(int row)
{
    Editor *e = editorGetHandle();
    row = row != -1 ? row : e->numLines;

    if (e->numLines >= e->lineCap)
    {
        // Realloc editor line buffer array when full
        e->lineCap += BUFFER_LINE_CAP;
        e->lines = memRealloc(e->lines, e->lineCap * sizeof(Line));
        check_pointer(e->lines, "bufferInsertLine");
    }

    if (row < e->numLines)
    {
        // Move lines down when adding newline mid-file
        Line *pos = e->lines + row;
        int count = e->numLines - row;
        memmove(pos + 1, pos, count * sizeof(Line));
    }

    Line line = {
        .chars = memZeroAlloc(DEFAULT_LINE_LENGTH * sizeof(char)),
        .cap = DEFAULT_LINE_LENGTH,
        .row = row,
        .length = 0,
        .idx = 0,
    };

    if (e->indent > 0)
    {
        memset(line.chars, ' ', e->indent);
        line.length = e->indent;
    }

    check_pointer(line.chars, "bufferCreateLine");
    memcpy(&e->lines[row], &line, sizeof(Line));
    e->numLines++;
    e->info.dirty = true;
}

// Reallocs line char buffer.
static void bufferExtendLine(int row, int new_size)
{
    Editor *e = editorGetHandle();
    Line *line = &e->lines[row];
    line->cap = new_size;
    line->chars = memRealloc(line->chars, line->cap);
    check_pointer(line->chars, "bufferExtendLine");
    memset(line->chars + line->length, 0, line->cap - line->length);
}

// Deletes line at row and move all lines below upwards.
void BufferDeleteLine(int row)
{
    Editor *e = editorGetHandle();
    if (row > e->numLines-1)
        return;

    if (row == 0 && e->numLines == 1)
    {
        memset(e->lines[row].chars, 0, e->lines[row].cap);
        e->lines[row].length = 0;
        return;
    }

    memFree(e->lines[row].chars);
    Line *pos = e->lines + row + 1;

    if (row != e->lineCap - 1)
    {
        int count = e->numLines - row;
        memmove(pos - 1, pos, count * sizeof(Line));
        memset(e->lines + e->numLines, 0, sizeof(Line));
    }

    e->numLines--;
    e->info.dirty = true;
}

// Copies and removes all characters behind the cursor position,
// then pastes them at the end of the line below.
void BufferSplitLineDown(int row)
{
    Editor *e = editorGetHandle();
    Line *from = &e->lines[row];
    Line *to = &e->lines[row + 1];
    int length = from->length - e->col;

    if (to->cap <= length)
    {
        // Realloc line buffer so new text fits
        int l = DEFAULT_LINE_LENGTH;
        bufferExtendLine(row + 1, (length / l) * l + l);
    }

    // Copy characters and set right side of row to 0
    strcpy(to->chars + to->length, from->chars + e->col);
    memset(from->chars + e->col + to->length, 0, length);
    to->length += length;
    from->length -= length;
    e->info.dirty = true;
}

// Moves line content from row to end of line above.
void BufferSplitLineUp(int row)
{
    Editor *e = editorGetHandle();
    Line *from = &e->lines[row];
    Line *to = &e->lines[row - 1];

    if (from->length == 0)
        return;

    int length = from->length - e->col + to->length;
    if (to->cap <= length)
    {
        // Realloc line buffer so new text fits
        int l = DEFAULT_LINE_LENGTH;
        bufferExtendLine(row - 1, (length / l) * l + l);
    }

    memcpy(to->chars + to->length, from->chars, from->length);
    to->length += from->length;
    e->info.dirty = true;
}

// Scrolls buffer vertically by delta y.
void BufferScroll(int dy)
{
    Editor *e = editorGetHandle();
    int real_y = (e->row - e->offy);

    // If cursor is scrolling up/down (within scroll threshold)
    if ((real_y > e->textH - e->scrollDy && dy > 0) ||
        (real_y < e->scrollDy && dy < 0))
        e->offy += dy;

    // Do not let scroll go past end of file
    if (e->offy + e->textH > e->numLines)
        e->offy = e->numLines - e->textH;

    // Do not scroll past beginning or if page is not filled
    if (e->offy < 0 || e->numLines <= e->textH)
        e->offy = 0;
}

// Shorthand for scrolling down by one. Moves cursor too.
void BufferScrollDown()
{
    Editor *e = editorGetHandle();
    if (e->row < e->numLines - 1 && e->numLines - e->offy >= e->height - 1)
    {
        e->offy++;
        e->row++;
    }
}

// Shorthand for scrolling up by one. Moves cursor too.
void BufferScrollUp()
{
    Editor *e = editorGetHandle();
    if (e->row > 1 && e->offy > 0)
    {
        e->offy--;
        e->row--;
    }
}