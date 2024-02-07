#include "wim.h"

extern Editor editor;

void SaveEditorAction(Action type, int row, int col, char *text)
{
    EditorAction action = {
        .type = type,
        .row = row,
        .col = col,
        .textLen = 0,
    };

    if (text != NULL)
    {
        strcpy(action.text, text);
        action.textLen = strlen(text);
    }

    append(editor.actions, &action);
}

void Undo()
{
    if (len(editor.actions) == 0)
        return;

    EditorAction *action = pop(editor.actions);

    switch (action->type)
    {
    case A_DELETE:
        CursorSetPos(action->col, action->row, false);
        BufferWrite(action->text, action->textLen);
        break;

    case A_WRITE:
        CursorSetPos(action->col + action->textLen, action->row, false);
        BufferDelete(action->textLen);
        break;

    default:
        break;
    }
}

void Redo()
{
}
