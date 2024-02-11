// All things related to writing, deleting, and modifying things in the current buffer.

#include "wim.h"

extern Editor editor;

// Reallocs line char buffer.
static void bufferExtendLine(int row, int new_size)
{
    Line *line = &editor.lines[row];
    line->cap = new_size;
    line->chars = memRealloc(line->chars, line->cap);
    check_pointer(line->chars, "bufferExtendLine");
    memset(line->chars + line->length, 0, line->cap - line->length);
}

// Writes characters to buffer at cursor pos. Does not filter out non-writable characters.
void BufferWrite(char *source, int length)
{
    Line *line = &editor.lines[editor.row];

    if (line->length + length >= line->cap)
    {
        // Allocate enough memory for the total string
        int l = DEFAULT_LINE_LENGTH;
        int requiredSpace = (length / l + 1) * l;
        bufferExtendLine(editor.row, line->cap + requiredSpace);
    }

    if (editor.col < line->length)
    {
        // Move text when typing in the middle of a line
        char *pos = line->chars + editor.col;
        memmove(pos + length, pos, line->length - editor.col);
    }

    memcpy(line->chars + editor.col, source, length);
    line->length += length;
    editor.col += length;
    editor.info.dirty = true;
}

// Same as BufferDeleteChar, but does not delete newlines.
void BufferDelete(int count)
{
    if (editor.col == 0)
        return;

    Line *line = &editor.lines[editor.row];
    count = min(count, editor.col); // Dont delete past 0

    if (editor.col <= line->length)
    {
        // Move chars when deleting in middle of line
        char *pos = line->chars + editor.col;
        memmove(pos - count, pos, line->length - editor.col);
    }

    memset(line->chars + line->length, 0, line->cap - line->length);
    line->length -= count;
    editor.col -= count;
    editor.info.dirty = true;
}

// Deletes the character before the cursor position.
void BufferDeleteChar()
{
    if (editor.col == 0)
        return;

    Line *line = &editor.lines[editor.row];

    if (editor.col <= line->length)
    {
        // Move chars when deleting in middle of line
        char *pos = line->chars + editor.col;
        memmove(pos - 1, pos, line->length - editor.col);
    }

    memset(line->chars + line->length, 0, line->cap - line->length);
    line->length -= 1;
    editor.col -= 1;
    editor.info.dirty = true;
}

// Returns number of spaces before the cursor
int BufferGetIndent()
{
    Line *line = &editor.lines[editor.row];
    int prefixedSpaces = 0;

    for (int i = editor.col - 1; i >= 0; i--)
    {
        if (line->chars[i] != ' ')
            break;
        prefixedSpaces++;
    }

    return prefixedSpaces;
}

// Inserts new line at row. If row is -1 line is appended to end of file.
void BufferInsertLine(int row)
{
    row = row != -1 ? row : editor.numLines;

    if (editor.numLines >= editor.lineCap)
    {
        // Realloc editor line buffer array when full
        editor.lineCap += BUFFER_LINE_CAP;
        editor.lines = memRealloc(editor.lines, editor.lineCap * sizeof(Line));
        check_pointer(editor.lines, "bufferInsertLine");
    }

    if (row < editor.numLines)
    {
        // Move lines down when adding newline mid-file
        Line *pos = editor.lines + row;
        int count = editor.numLines - row;
        memmove(pos + 1, pos, count * sizeof(Line));
    }

    Line line = {
        .chars = memZeroAlloc(DEFAULT_LINE_LENGTH * sizeof(char)),
        .cap = DEFAULT_LINE_LENGTH,
        .row = row,
        .length = 0,
    };

    if (editor.indent > 0)
    {
        memset(line.chars, ' ', editor.indent);
        line.length = editor.indent;
    }

    check_pointer(line.chars, "bufferCreateLine");
    memcpy(&editor.lines[row], &line, sizeof(Line));
    editor.numLines++;
    editor.info.dirty = true;
}

// Deletes line at row and move all lines below upwards.
void BufferDeleteLine(int row)
{
    if (row > editor.numLines - 1)
        return;

    Line *line = &editor.lines[row];

    if (row == 0 && editor.numLines == 1)
    {
        memset(line->chars, 0, line->cap);
        line->length = 0;
        return;
    }

    memFree(line->chars);
    Line *pos = editor.lines + row + 1;

    if (row != editor.lineCap - 1)
    {
        int count = editor.numLines - row;
        memmove(pos - 1, pos, count * sizeof(Line));
        memset(editor.lines + editor.numLines, 0, sizeof(Line));
    }

    editor.numLines--;
    editor.info.dirty = true;
}

// Copies and removes all characters behind the cursor position,
// then pastes them at the end of the line below.
void BufferSplitLineDown(int row)
{
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
    editor.info.dirty = true;
}

// Moves line content from row to end of line above.
void BufferSplitLineUp(int row)
{
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
    editor.info.dirty = true;
}

// Scrolls buffer vertically by delta y.
void BufferScroll(int dy)
{
    int real_y = (editor.row - editor.offy);

    // If cursor is scrolling up/down (within scroll threshold)
    if ((real_y > editor.textH - editor.scrollDy && dy > 0) ||
        (real_y < editor.scrollDy && dy < 0))
        editor.offy += dy;

    // Do not let scroll go past end of file
    if (editor.offy + editor.textH > editor.numLines)
        editor.offy = editor.numLines - editor.textH;

    // Do not scroll past beginning or if page is not filled
    if (editor.offy < 0 || editor.numLines <= editor.textH)
        editor.offy = 0;
}

// Shorthand for scrolling down by one. Moves cursor too.
void BufferScrollDown()
{
    if (editor.row < editor.numLines - 1 && editor.numLines - editor.offy >= editor.height - 1)
    {
        editor.offy++;
        editor.row++;
    }
}

// Shorthand for scrolling up by one. Moves cursor too.
void BufferScrollUp()
{
    if (editor.row > 1 && editor.offy > 0)
    {
        editor.offy--;
        editor.row--;
    }
}
