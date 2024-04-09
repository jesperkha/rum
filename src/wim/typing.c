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
        BufferWrite(currentBuffer, &c, 1);
        CursorMove(currentBuffer, 1, 0);
        if (config.matchParen)
            matchParen(c);
    }
}

// Writes text after cursor pos.
void TypingWrite(char *source, int length)
{
    BufferWrite(currentBuffer, source, length);
    CursorMove(currentBuffer, length, 0);
}

// Deletes a single character before the cursor, or more if deleting a tab.
void TypingBackspace()
{
    if (currentCol == 0)
    {
        // Delete line if there are more than one lines
        if (currentRow != 0)
        {
            int length = BufferMoveTextUp(currentBuffer);
            CursorSetPos(currentBuffer, length, currentRow - 1, false);
            BufferDeleteLine(currentBuffer, currentRow + 1);
        }
        return;
    }

    int indent = BufferGetIndent(currentBuffer);
    if (indent > 0 && indent % config.tabSize == 0)
    {
        // Delete tab if prefixed whitespace is >= tabsize
        BufferDelete(currentBuffer, config.tabSize);
        CursorMove(currentBuffer, -config.tabSize, 0);
        return;
    }

    // Delete char
    BufferDelete(currentBuffer, 1);
    CursorMove(currentBuffer, -1, 0);
}

// Inserts newline while keeping indentation cursor position.
void TypingNewline()
{
    BufferInsertLine(currentBuffer, currentBuffer->cursor.row + 1);
    BufferMoveTextDown(currentBuffer);
    CursorSetPos(currentBuffer, 0, currentRow + 1, false);
    if (config.matchParen)
        breakParen();
}

void TypingDeleteLine()
{
    BufferDeleteLine(currentBuffer, currentRow);
    CursorMove(currentBuffer, 0, 0); // Just update
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
        if (c == ends[i] && currentChar == ends[i])
        {
            TypingDelete();
            break;
        }

        if (c == begins[i])
        {
            if (begins[i] == ends[i])
                BufferWrite(currentBuffer, (char *)&ends[i], 1);
            else
                TypingWriteChar(ends[i]);

            CursorMove(currentBuffer, -1, 0);
            break;
        }
    }
}

// Moves paren down and indents line when pressing enter after a paren.
static void breakParen()
{
    Line line2 = currentBuffer->lines[currentRow - 1];

    for (int i = 2; i < strlen(begins); i++)
    {
        char a = begins[i];
        char b = ends[i];

        if (line2.chars[line2.length - 1] != a)
            continue;

        if (currentChar == b)
        {
            TypingNewline();
            CursorMove(currentBuffer, 0, -1);
            TypingInsertTab();
        }

        return;
    }
}

// Deletes one character to the right.
void TypingDelete()
{
    if (currentCol == currentLine.length)
    {
        if (currentRow == currentBuffer->numLines - 1)
            return;
        CursorSetPos(currentBuffer, 0, currentRow + 1, false);
    }
    else
        CursorMove(currentBuffer, 1, 0);

    TypingBackspace();
}