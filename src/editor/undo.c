#include "wim.h"

extern Editor editor;

static Action lastAction = A_CURSOR;

void UndoSaveAction(Action type, char *text, int textLen)
{
    UndoSaveActionEx(type, editor.row, editor.col, text, textLen);
}

void UndoSaveActionEx(Action type, int row, int col, char *text, int textLen)
{
    if (type == A_CURSOR) // Is not added to stack
        goto _return;

    // Combine actions to undo more at a time.
    if (lastAction == type)
    {
        EditorAction *last = &editor.actions[len(editor.actions) - 1];

        // Typing actions - append string to buffer
        if (type == A_WRITE || type == A_DELETE || type == A_BACKSPACE)
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

                if (type == A_WRITE || type == A_DELETE)
                    last->endCol += textLen;
                if (type == A_BACKSPACE)
                    last->endCol -= textLen;

                goto _return;
            }
        }
    }

    EditorAction action = {
        .type = type,
        .row = row,
        .col = col,
        .textLen = textLen,
        .endCol = col,
    };

    if (text != NULL)
    {
        // Todo: buf resize for long lines
        strncpy(action.text, text, textLen);
        action.textLen = textLen;

        if (type == A_DELETE || type == A_WRITE || type == A_BACKSPACE)
        {
            // Endcol is where the action should be performed (delete/write)
            int dir = type == A_BACKSPACE ? -1 : 1; // Else A_WRITE or A_DELETE
            action.endCol = col + action.textLen * dir;
        }
    }

    append(editor.actions, &action);

_return:
    lastAction = type;
}

// Joins last n actions under same undo call.
void UndoJoin(int n)
{
    UndoSaveAction(A_JOIN, NULL, n);
}

static void undoAction(EditorAction *action)
{
    switch (action->type)
    {
    case A_BACKSPACE:
        strrev(action->text);
    case A_DELETE:
        CursorSetPos(action->endCol, action->row, false);
        BufferWrite(action->text, action->textLen);
        break;

    case A_WRITE:
        CursorSetPos(action->endCol, action->row, false);
        BufferDelete(action->textLen);
        break;

    case A_DELETE_LINE:
    {
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

    CursorSetPos(action->col, action->row, false);
    lastAction = A_UNDO; // Prevent read from action before undo
}

void Undo()
{
    if (len(editor.actions) == 0)
        return;

    EditorAction *action = pop(editor.actions);

    if (action->type == A_JOIN)
    {
        for (int i = 0; i < action->textLen; i++)
        {
            EditorAction *action = pop(editor.actions);
            undoAction(action);
        }
    }
    else
        undoAction(action);
}

void Redo()
{
}
