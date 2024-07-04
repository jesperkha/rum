// All the typing functionality wim features out of the box neatly packed into one API/file.

#include "wim.h"

#define BLANKS "        " // 8

extern Editor editor;
extern Config config;

static void matchParen(char c);
static void breakParen();

// Writes a single character to buffer if valid.
void TypingWriteChar(char c)
{
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
    BufferWrite(curBuffer, source, length);
    UndoSaveAction(A_WRITE, source, length);
    CursorMove(curBuffer, length, 0);
}

// Deletes a single character before the cursor, or more if deleting a tab.
void TypingBackspace()
{
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
        UndoSaveAction(A_BACKSPACE, BLANKS, config.tabSize);
        return;
    }

    // Delete char
    UndoSaveActionEx(A_BACKSPACE, curRow, curCol - 1, &curLine.chars[curCol - 1], 1);
    BufferDelete(curBuffer, 1);
    CursorMove(curBuffer, -1, 0);
}

void TypingBackspaceMany(int count)
{
    if (curCol - count < 0)
        count = curCol;
    UndoSaveActionEx(A_DELETE_BACK, curRow, curCol - count, &curLine.chars[curCol - count], count);
    BufferDelete(curBuffer, count);
    CursorMove(curBuffer, -count, 0);
}

// Inserts newline while keeping indentation cursor position.
void TypingNewline()
{
    int pos = curLine.indent;
    UndoSaveActionEx(A_INSERT_LINE, curRow + 1, curCol, curLine.chars, curLine.length);
    BufferInsertLine(curBuffer, curRow + 1);
    BufferMoveTextDown(curBuffer);
    CursorSetPos(curBuffer, pos, curRow + 1, false);
    if (config.matchParen)
        breakParen();
}

void TypingDeleteLine()
{
    UndoSaveAction(A_DELETE_LINE, curLine.chars, curLine.length);
    BufferDeleteLine(curBuffer, curRow);
    CursorMove(curBuffer, 0, 0); // Just update
}

// Inserts tab according to current editor tab size config.
void TypingInsertTab()
{
    int tabs = min(config.tabSize, 8);
    TypingWrite(BLANKS, tabs);
}

// Order sensitive
static const char begins[] = "\"'({[";
static const char ends[] = "\"')}]";

// Matches braces, parens, strings etc. Also removes extra closing brackets
// when typing them out back to back, eg. ()
static void matchParen(char c)
{
    for (int i = 0; i < strlen(begins); i++)
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

    for (int i = 2; i < strlen(begins); i++)
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
    if (curLine.length - curCol < count)
        count = curLine.length - curCol;
    if (count <= 0)
        return;
    UndoSaveActionEx(A_DELETE, curRow, curCol, &curLine.chars[curCol], count);
    CursorMove(curBuffer, count, 0);
    BufferDelete(curBuffer, count);
    CursorMove(curBuffer, -count, 0);
}

void TypingClearLine()
{
    CursorSetPos(curBuffer, curLine.indent, curRow, false);
    TypingDeleteMany(curLine.length);
}