// Typing helpers considered part of the base editor functionality.

#include "wim.h"

extern Editor editor;

// Writes a single character to buffer if valid.
void TypingWriteChar(char c)
{
    if (!(c < 32 || c > 126))
    {
        // SaveEditorAction(A_WRITE, &c, 1);
        BufferWrite(&c, 1);
        if (editor.config.matchParen)
            TypingMatchParen(c);
    }
}

// Deletes a single character before the cursor.
void TypingDeleteChar()
{
    if (editor.col > 0)
    {
        // char c = editor.lines[editor.row].chars[editor.col - 1];
        // SaveEditorAction(A_DELETE, &c, 1);
        BufferDeleteChar();
    }
    else
    {
        if (editor.row == 0)
            return;

        // Delete line if cursor is at start
        // Line line = editor.lines[editor.row];
        // SaveEditorAction(A_DELETE_LINE, line.chars, line.length);

        int row = editor.row;
        int length = editor.lines[editor.row - 1].length;

        CursorSetPos(length, editor.row - 1, false);
        BufferSplitLineUp(row);
        BufferDeleteLine(row);
        CursorSetPos(length, editor.row, false);
    }
}

// Inserts newline while keeping indentation cursor position.
void TypingNewline()
{
    // SaveEditorAction(A_INSERT_LINE, NULL, 0);
    BufferInsertLine(editor.row + 1);
    int length = editor.lines[editor.row + 1].length;
    BufferSplitLineDown(editor.row);
    CursorSetPos(length, editor.row + 1, false);
    if (editor.config.matchParen)
        TypingBreakParen();
}

void TypingDeleteLine()
{
    // Line line = editor.lines[editor.row];
    // SaveEditorAction(A_DELETE_LINE, line.chars, line.length);
    BufferDeleteLine(editor.row);
    CursorSetPos(0, editor.row, true);
}

// Note: order sensitive
static const char begins[] = "\"'({[";
static const char ends[] = "\"')}]";

// Inserts tab according to current editor tab size config.
void TypingInsertTab()
{
    char *spaces = "        "; // 8
    int tabs = min(editor.config.tabSize, 8);
    // SaveEditorAction(A_WRITE, spaces, tabs);
    BufferWrite(spaces, tabs);
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
            TypingWriteChar(ends[i]);
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
                // SaveEditorAction(A_INSERT_LINE, NULL, 0);
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

    TypingDeleteChar();
    CursorShow();
}