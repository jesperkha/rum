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
        // if (config.matchParen)
        //     matchParen(c);
    }
}

// Writes text after cursor pos.
void TypingWrite(char *source, int length)
{
    BufferWrite(curBuffer, source, length);
    CursorMove(curBuffer, length, 0);
}

// Deletes a single character before the cursor, or more if deleting a tab.
void TypingBackspace()
{
    if (curCol == 0)
    {
        // Delete line if there are more than one lines
        if (curRow != 0)
        {
            int length = BufferMoveTextUp(curBuffer);
            CursorSetPos(curBuffer, length, curRow - 1, false);
            BufferDeleteLine(curBuffer, curRow + 1);
        }
        return;
    }

    int indent = BufferGetIndent(curBuffer);
    if (indent > 0 && indent % config.tabSize == 0)
    {
        // Delete tab if prefixed whitespace is >= tabsize
        BufferDelete(curBuffer, config.tabSize);
        CursorMove(curBuffer, -config.tabSize, 0);
        return;
    }

    // Delete char
    BufferDelete(curBuffer, 1);
    CursorMove(curBuffer, -1, 0);
}

// Inserts newline while keeping indentation cursor position.
void TypingNewline()
{
    int pos = curLine.indent;
    BufferInsertLine(curBuffer, curBuffer->cursor.row + 1);
    BufferMoveTextDown(curBuffer);
    CursorSetPos(curBuffer, pos, curRow + 1, false);
    if (config.matchParen)
        breakParen();
}

void TypingDeleteLine()
{
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

    TypingBackspace();
}