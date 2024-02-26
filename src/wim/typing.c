// All the typing functionality wim features out of the box neatly packed into one API/file.

#include "wim.h"

#define BLANKS "        " // 8

extern Editor editor;
extern Buffer buffer;

// Writes a single character to buffer if valid.
void TypingWriteChar(char c)
{
    // if (!(c < 32 || c > 126))
    // {
    //     UndoSaveAction(A_WRITE, &c, 1);
    //     BufferWrite(&buffer, &c, 1);
    //     if (editor.config.matchParen)
    //         TypingMatchParen(c);
    // }
}

// Writes text after cursor pos.
void TypingWrite(char *source, int length)
{
}

// Deletes a single character before the cursor, or more if deleting a tab.
void TypingBackspace()
{
    // LogNumber("indent", editor.config.tabSize);
    // if (buffer.cursor.col > 0)
    // {
    //     int indent = BufferGetIndent(&buffer);
    //     int tabSize = editor.config.tabSize;
    //     Line line = buffer.lines[buffer.cursor.row];

    //     if (indent > 0 && indent % tabSize == 0)
    //     {
    //         // Delete tab
    //         UndoSaveAction(A_BACKSPACE, BLANKS, tabSize);
    //         BufferDelete(&buffer, tabSize);
    //     }
    //     else
    //     {
    //         UndoSaveAction(A_BACKSPACE, &line.chars[buffer.cursor.col - 1], 1);
    //         BufferDelete(&buffer, 1);
    //     }
    // }
    // else
    // {
    //     if (buffer.cursor.row == 0)
    //         return;

    //     // Delete line if cursor is at start
    //     Line line = buffer.lines[buffer.cursor.row];
    //     Line above = buffer.lines[buffer.cursor.row - 1];
    //     UndoSaveAction(A_DELETE_LINE, line.chars, line.length);
    //     UndoSaveActionEx(A_WRITE, buffer.cursor.row - 1, buffer.cursor.col + above.length, line.chars, line.length);
    //     UndoJoin(2);

    //     int row = buffer.cursor.row;
    //     int length = buffer.lines[buffer.cursor.row - 1].length;
    //     CursorSetPos(length, buffer.cursor.row - 1, false);
    //     BufferMoveTextUpEx(&buffer, row, buffer.cursor.col);
    //     BufferDeleteLine(&buffer, row);
    //     CursorSetPos(length, buffer.cursor.row, false);
    // }
}

// Inserts newline while keeping indentation cursor position.
void TypingNewline()
{
    // Line line = buffer.lines[buffer.cursor.row];
    // UndoSaveAction(A_DELETE, line.chars + buffer.cursor.col, line.length - buffer.cursor.col);
    // UndoSaveActionEx(A_INSERT_LINE, buffer.cursor.row + 1, buffer.cursor.col, NULL, 0);
    // UndoJoin(2);

    // BufferInsertLine(&buffer, buffer.cursor.row + 1, NULL);
    // int length = buffer.lines[buffer.cursor.row + 1].length;
    // BufferMoveTextDown(&buffer);
    // CursorSetPos(length, buffer.cursor.row + 1, false);
    // if (editor.config.matchParen)
    //     TypingBreakParen();
}

void TypingDeleteLine()
{
    // Line line = buffer.lines[buffer.cursor.row];
    // if (buffer.numLines == 1)
    //     UndoSaveAction(A_DELETE, line.chars, line.length);
    // else
    //     UndoSaveAction(A_DELETE_LINE, line.chars, line.length);
    // BufferDeleteLine(&buffer, buffer.cursor.row);
    // CursorSetPos(0, buffer.cursor.row, true);
}

// Note: order sensitive
// static const char begins[] = "\"'({[";
// static const char ends[] = "\"')}]";

// Inserts tab according to current editor tab size config.
void TypingInsertTab()
{
    // int tabs = min(editor.config.tabSize, 8);
    // UndoSaveAction(A_WRITE, BLANKS, tabs);
    // BufferWrite(&buffer, BLANKS, tabs);
}

// Matches braces, parens, strings etc. Also removes extra closing brackets
// when typing them out back to back, eg. ()
// static void typingMatchParen(char c)
// {
//     Line line = buffer.lines[buffer.cursor.row];

//     for (int i = 0; i < strlen(begins); i++)
//     {
//         if (c == ends[i] && line.chars[buffer.cursor.col] == ends[i])
//         {
//             TypingDelete();
//             UndoJoin(3); // Undo initial paren too
//             break;
//         }

//         if (c == begins[i])
//         {
//             if (begins[i] == ends[i])
//             {
//                 UndoSaveAction(A_WRITE, "+", 1);
//                 BufferWrite(&buffer, (char *)&ends[i], 1);
//             }
//             else
//                 TypingWriteChar(ends[i]);

//             CursorMove(-1, 0);
//             break;
//         }
//     }
// }

// Moves paren down and indents line when pressing enter after a paren.
// static void typingBreakParen()
// {
//     Line line1 = buffer.lines[buffer.cursor.row];
//     Line line2 = buffer.lines[buffer.cursor.row - 1];

//     for (int i = 2; i < strlen(begins); i++)
//     {
//         char a = begins[i];
//         char b = ends[i];

//         if (line2.chars[line2.length - 1] == a)
//         {
//             TypingInsertTab();
//             UndoSaveAction(A_INSERT_LINE, NULL, 0);

//             if (line1.chars[buffer.cursor.col] == b)
//             {
//                 BufferInsertLine(&buffer, buffer.cursor.row + 1, NULL);
//                 BufferMoveTextDown(&buffer);
//             }

//             UndoJoin(5); // Include original breakline
//             return;
//         }
//     }
// }

// Deletes one character to the right.
void TypingDelete()
{
    // if (buffer.cursor.col == buffer.lines[buffer.cursor.row].length)
    // {
    //     if (buffer.cursor.row == buffer.numLines - 1)
    //         return;

    //     CursorHide();
    //     CursorSetPos(0, buffer.cursor.row + 1, false);
    // }
    // else
    // {
    //     CursorHide();
    //     CursorMove(1, 0);
    // }

    // // Todo: fix undo for delete
    // TypingBackspace();
    // CursorShow();
}