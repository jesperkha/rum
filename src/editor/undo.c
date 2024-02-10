#include "wim.h"

extern Editor editor;

static Action lastAction = A_CURSOR;

void SaveEditorAction(Action type, char *text)
{
    int row = editor.row, col = editor.col;

    // _return sets lastAction at func end

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
                last->textLen + strlen(text) < ACTION_BUFSIZE - 1)
            {
                // Append text to end of buffer. DELETE is reversed on undo.
                strcat(last->text, text);
                last->textLen = strlen(last->text);

                if (type == A_WRITE)
                    last->endCol += strlen(text);
                if (type == A_DELETE)
                    last->endCol -= strlen(text);

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

    switch (type)
    {
    case A_WRITE:
    case A_DELETE:
    {
        if (text != NULL)
        {
            strcpy(action.text, text);
            action.textLen = strlen(text);

            // Endcol is where the action should be performed (delete/write)
            int dir = type == A_DELETE ? -1 : 1; // Else A_WRITE
            action.endCol = col + action.textLen * dir;
        }
    }

    default:
        break;
    }

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
        CursorSetPos(action->endCol - 1, action->row, false);
        BufferDelete(action->textLen);
        break;

    default:
        break;
    }
}

void Redo()
{
}
