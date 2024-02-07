// Typing helpers considered part of the base editor functionality.

#include "wim.h"

extern Editor editor;

// Writes a single character to buffer if valid.
void TypingWriteChar(char c)
{
    if (!(c < 32 || c > 126))
    {
        BufferWrite(&c, 1);
        if (editor.config.matchParen)
            TypingMatchParen(c);

        char str[2] = {c, 0};
        SaveEditorAction(A_WRITE, editor.row, editor.col, str);
    }
}

// Deletes a single character before the cursor.
void TypingDeleteChar()
{
    if (editor.col > 0)
    {
        char c = editor.lines[editor.row].chars[editor.col - 1];
        char str[2] = {c, 0};
        SaveEditorAction(A_DELETE, editor.row, editor.col, str);
    }
    else if (editor.row > 0)
    {
        // Newline action
    }

    BufferDeleteChar();
}

// Inserts newline while keeping indentation cursor position.
void TypingNewline()
{
    BufferInsertLine(editor.row + 1);
    int length = editor.lines[editor.row + 1].length;
    BufferSplitLineDown(editor.row);
    CursorSetPos(length, editor.row + 1, false);
    if (editor.config.matchParen)
        TypingBreakParen();
}

// Note: order sensitive
static const char begins[] = "\"'({[";
static const char ends[] = "\"')}]";

// Inserts tab according to current editor tab size config.
void TypingInsertTab()
{
    char *spaces = "        "; // 8
    BufferWrite(spaces, min(editor.config.tabSize, 8));
}

// Matches braces, parens, strings etc. Also removes extra closing brackets
// when typing them out back to back, eg. ()
void TypingMatchParen(char c)
{
    Line line = editor.lines[editor.row];

    for (int i = 0; i < strlen(begins); i++)
    {
        if (c == ends[i] && line.chars[editor.col] == ends[i])
        {
            TypingDeleteForward();
            break;
        }

        if (c == begins[i])
        {
            BufferWrite((char *)&ends[i], 1);
            CursorMove(-1, 0);
            break;
        }
    }
}

// Moves paren down and indents line when pressing enter after a paren.
void TypingBreakParen()
{
    Line line1 = editor.lines[editor.row];
    Line line2 = editor.lines[editor.row - 1];

    for (int i = 2; i < strlen(begins); i++)
    {
        char a = begins[i];
        char b = ends[i];

        if (line2.chars[line2.length - 1] == a)
        {
            TypingInsertTab();

            if (line1.chars[editor.col] == b)
            {
                BufferInsertLine(editor.row + 1);
                BufferSplitLineDown(editor.row);
            }

            return;
        }
    }
}

// Deletes one character to the right.
void TypingDeleteForward()
{
    if (editor.col == editor.lines[editor.row].length)
    {
        if (editor.row == editor.numLines - 1)
            return;

        CursorHide();
        CursorSetPos(0, editor.row + 1, false);
    }
    else
    {
        CursorHide();
        CursorMove(1, 0);
    }

    BufferDeleteChar();
    CursorShow();
}