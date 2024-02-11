#include "wim.h"

extern Editor editor;

static Action lastAction = A_CURSOR;

void SaveEditorAction(Action type, char *text, int textLen)
{
    int row = editor.row, col = editor.col;

    if (type == A_CURSOR) // Is not added to stack
        goto _return;

    // Combine actions to undo more at a time.
    if (lastAction == type)
    {
        EditorAction *last = &editor.actions[len(editor.actions) - 1];

        // Typing actions - append string to buffer
        if (type == A_WRITE || type == A_DELETE)
        {
            if (
                // If text is one char away and on the same line, and theres still space left
                last->endCol == col &&
                last->row == row &&
                text != NULL &&
                *text != ' ' &&
                last->textLen + textLen < ACTION_BUFSIZE - 1)
            {
                // Append text to end of buffer. DELETE is reversed on undo.
                strncat(last->text, text, textLen);
                last->textLen += textLen;

                if (type == A_WRITE)
                    last->endCol += textLen;
                if (type == A_DELETE)
                    last->endCol -= textLen;

                goto _return;
            }
        }
    }

    EditorAction action = {
        .type = type,
        .row = row,
        .col = col,
        .textLen = 0,
        .endCol = col,
    };

    if (text != NULL)
    {
        // Todo: buf resize for long lines
        strncpy(action.text, text, textLen);
        action.textLen = textLen;

        if (type == A_DELETE || type == A_WRITE)
        {
            // Endcol is where the action should be performed (delete/write)
            int dir = type == A_DELETE ? -1 : 1; // Else A_WRITE
            action.endCol = col + action.textLen * dir;
        }
    }

    LogNumber("Action", type);
    append(editor.actions, &action);

_return:
    lastAction = type;
}

void Undo()
{
    if (len(editor.actions) == 0)
        return;

    EditorAction *action = pop(editor.actions);

    switch (action->type)
    {
    case A_DELETE:
        CursorSetPos(action->endCol, action->row, false);
        strrev(action->text);
        BufferWrite(action->text, action->textLen);
        break;

    case A_WRITE:
        CursorSetPos(action->endCol, action->row, false);
        BufferDelete(action->textLen);
        break;

    case A_DELETE_LINE:
    {
        if (action->row > 0) // First line is always there
            BufferInsertLine(action->row);
        CursorSetPos(action->col, action->row, false);
        BufferWrite(action->text, action->textLen);
        break;
    }

    case A_INSERT_LINE:
        CursorSetPos(action->col, action->row, false);
        BufferDeleteLine(action->row);
        break;

    default:
        break;
    }
}

void Redo()
{
}
