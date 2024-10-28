#include "rum.h"

extern Editor editor;
extern Config config;

static void matchParen(char c);
static void breakParen();

// Writes a single character to buffer if valid.
void TypingWriteChar(char c)
{
    if (curBuffer->readOnly)
        return;

    if (!(c < 32 || c > 126))
    {
        BufferWrite(curBuffer, &c, 1);
        UndoSaveAction(A_WRITE, &c, 1);
        CursorMove(curBuffer, 1, 0);
        if (config.matchParen)
            matchParen(c);
    }
}

// Writes text after cursor pos.
void TypingWrite(char *source, int length)
{
    if (curBuffer->readOnly)
        return;
    BufferWrite(curBuffer, source, length);
    UndoSaveAction(A_WRITE, source, length);
    CursorMove(curBuffer, length, 0);
}

void TypingWriteMultiline(char *source)
{
    char *p = strtok(source, "\n");
    int undoCount = 0;

    while (p)
    {
        int len = strlen(p);
        if (*(p + len - 1) == '\r')
            len--;

        TypingWrite(p, len);
        undoCount++;
        p = strtok(NULL, "\n");

        if (p != NULL)
        {
            UndoSaveActionEx(A_INSERT_LINE, curRow + 1, 0, curLine.chars, curLine.length);
            BufferInsertLine(curBuffer, curRow + 1);
            CursorMove(curBuffer, 0, 1);
            undoCount++;
        }
    }

    UndoJoin(undoCount);
}

// Deletes a single character before the cursor, or more if deleting a tab.
void TypingBackspace()
{
    if (curBuffer->readOnly)
        return;
    if (curCol == 0)
    {
        if (curRow == 0)
            return;

        // Delete line if there are more than one lines
        Line deleted = curBuffer->lines[curRow];
        UndoSaveActionEx(A_DELETE_LINE, curRow, 0, deleted.chars, deleted.length);

        int length = BufferMoveTextUp(curBuffer);
        UndoSaveActionEx(A_WRITE, curRow - 1, length, curLine.chars, curLine.length);
        CursorSetPos(curBuffer, length, curRow - 1, false);
        BufferDeleteLine(curBuffer, curRow + 1);

        UndoJoin(2);
        return;
    }

    int indent = BufferGetPrefixedSpaces(curBuffer);
    if (indent > 0 && indent % config.tabSize == 0)
    {
        // Delete tab if prefixed whitespace is >= tabsize
        BufferDelete(curBuffer, config.tabSize);
        CursorMove(curBuffer, -config.tabSize, 0);
        UndoSaveAction(A_BACKSPACE, editor.padBuffer, config.tabSize);
        return;
    }

    // Delete char
    UndoSaveActionEx(A_BACKSPACE, curRow, curCol - 1, &curLine.chars[curCol - 1], 1);
    BufferDelete(curBuffer, 1);
    CursorMove(curBuffer, -1, 0);
}

void TypingBackspaceMany(int count)
{
    if (curBuffer->readOnly)
        return;
    if (curCol - count < 0)
        count = curCol;

    char text[count + 1];
    memcpy(text, curLine.chars + curCol, count);
    text[count] = 0;
    SetClipboardText(text);

    UndoSaveActionEx(A_DELETE_BACK, curRow, curCol - count, &curLine.chars[curCol - count], count);
    BufferDelete(curBuffer, count);
    CursorMove(curBuffer, -count, 0);
}

// Inserts newline while keeping indentation cursor position.
void TypingNewline()
{
    if (curBuffer->readOnly)
        return;
    int pos = curLine.indent;
    UndoSaveActionEx(A_INSERT_LINE, curRow + 1, curCol, curLine.chars, curLine.length);
    BufferInsertLine(curBuffer, curRow + 1);
    BufferWriteEx(curBuffer, curRow + 1, 0, editor.padBuffer, pos);
    BufferMoveTextDown(curBuffer);
    CursorSetPos(curBuffer, pos, curRow + 1, false);
    if (config.matchParen)
        breakParen();
}

void TypingDeleteLine()
{
    if (curBuffer->readOnly)
        return;

    SetClipboardText(curLine.chars);
    UndoSaveAction(A_DELETE_LINE, curLine.chars, curLine.length);
    BufferDeleteLine(curBuffer, curRow);
    CursorMove(curBuffer, 0, 0); // Just update
}

// Inserts tab according to current editor tab size config.
void TypingInsertTab()
{
    if (curBuffer->readOnly)
        return;
    int tabs = min(config.tabSize, 8);
    TypingWrite(editor.padBuffer, tabs);
}

// Order sensitive
static const char begins[] = "\"'({[";
static const char ends[] = "\"')}]";

// Matches braces, parens, strings etc. Also removes extra closing brackets
// when typing them out back to back, eg. ()
static void matchParen(char c)
{
    for (int i = 0; i < (int)strlen(begins); i++)
    {
        if (c == ends[i] && curChar == ends[i])
        {
            TypingDelete();
            break;
        }

        if (c == begins[i])
        {
            if (begins[i] == ends[i])
            {
                BufferWrite(curBuffer, (char *)&ends[i], 1);
                UndoSaveAction(A_WRITE, (char *)&ends[i], 1);
                CursorMove(curBuffer, 1, 0);
            }
            else
                TypingWriteChar(ends[i]);

            CursorMove(curBuffer, -1, 0);
            break;
        }
    }
}

// Moves paren down and indents line when pressing enter after a paren.
static void breakParen()
{
    Line line2 = curBuffer->lines[curRow - 1];

    for (int i = 2; i < (int)strlen(begins); i++)
    {
        char a = begins[i];
        char b = ends[i];

        if (line2.chars[line2.length - 1] != a)
            continue;

        if (curChar == b)
        {
            TypingNewline();
            CursorMove(curBuffer, 0, -1);
            TypingInsertTab();
            UndoJoin(3); // 2x newline and tab
        }

        return;
    }
}

// Deletes one character to the right.
void TypingDelete()
{
    if (curBuffer->readOnly)
        return;
    if (curCol == curLine.length)
    {
        if (curRow == curBuffer->numLines - 1)
            return;
        CursorSetPos(curBuffer, 0, curRow + 1, false);
    }
    else
        CursorMove(curBuffer, 1, 0);

    UndoSaveActionEx(A_CURSOR, curRow, curCol - 1, "", 0);
    TypingBackspace();
    UndoJoin(2);
}

void TypingDeleteMany(int count)
{
    if (curBuffer->readOnly)
        return;
    if (curLine.length - curCol < count)
        count = curLine.length - curCol;
    if (count <= 0)
        return;

    char text[count + 1];
    memcpy(text, curLine.chars + curCol, count);
    text[count] = 0;
    SetClipboardText(text);

    UndoSaveActionEx(A_DELETE, curRow, curCol, &curLine.chars[curCol], count);
    CursorMove(curBuffer, count, 0);
    BufferDelete(curBuffer, count);
    CursorMove(curBuffer, -count, 0);
}

void TypingClearLine()
{
    if (curBuffer->readOnly)
        return;
    CursorSetPos(curBuffer, curLine.indent, curRow, false);
    TypingDeleteMany(curLine.length);
}

// Deletes marked text/lines in visual mode
void TypingDeleteMarked()
{
    if (curBuffer->readOnly)
        return;

    CopyToClipboard();

    CursorPos from, to;
    BufferOrderHighlightPoints(curBuffer, &from, &to);

    int undoCount = 0;

    for (int i = from.row; i <= to.row; i++)
    {
        Line line = curBuffer->lines[i];
        int start = from.row == i ? from.col : 0;
        int end = to.row == i ? to.col : line.length;

        if (line.length == 0 || (start == 0 && end == line.length))
        {
            UndoSaveActionEx(A_DELETE_LINE, i, 0, line.chars, line.length);
            BufferDeleteLine(curBuffer, i);
            undoCount++;
            i--;
            to.row--;
        }
        else
        {
            UndoSaveActionEx(A_DELETE, i, start, line.chars + start, end - start);
            BufferDeleteEx(curBuffer, i, end, end - start);
            undoCount++;
        }
    }

    UndoJoin(undoCount);
}

void TypingCommentOutLine()
{
    TypingCommentOutLines(curRow, curRow);
}

void TypingCommentOutLines(int from, int to)
{
    Assert(curBuffer->numLines > from && curBuffer->numLines > to);

    // Get the minimum indent which is not 0
    int lineBegin = 0xFFFF;
    for (int i = from; i <= to; i++)
    {
        Line line = curBuffer->lines[i];
        if (line.indent < lineBegin && line.length > 0)
            lineBegin = line.indent;
    }

    if (lineBegin == 0xFFFF) // Empty line
        return;

    char comment[] = "//"; // Todo: comment in lang config
    int commentLen = strlen(comment);

    bool commentOut = true;
    bool isFirst = true;
    int changed = 0; // Number of changes for undo join

    for (int i = from; i <= to; i++)
    {
        Line line = curBuffer->lines[i];
        if (line.length == 0)
            continue;

        if (isFirst)
        {
            // First line determines if the rest of the lines should be commented out or uncommented
            commentOut = line.length > lineBegin + commentLen && !strncmp(line.chars + lineBegin, comment, commentLen);
            isFirst = false;
        }

        if (commentOut)
        {
            UndoSaveActionEx(A_DELETE, i, lineBegin, comment, commentLen);
            BufferDeleteEx(curBuffer, i, lineBegin + commentLen, commentLen);
            if (line.chars[lineBegin] == ' ')
            {
                UndoSaveActionEx(A_DELETE, i, lineBegin, " ", 1);
                BufferDeleteEx(curBuffer, i, lineBegin + 1, 1);
                changed++;
            }
            changed++;
        }
        else
        {
            UndoSaveActionEx(A_WRITE, i, lineBegin, " ", 1);
            UndoSaveActionEx(A_WRITE, i, lineBegin, comment, commentLen);
            BufferWriteEx(curBuffer, i, lineBegin, comment, commentLen);
            BufferWriteEx(curBuffer, i, lineBegin + commentLen, " ", 1);
            changed += 2;
        }
    }

    UndoJoin(changed);
}