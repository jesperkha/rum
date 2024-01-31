#include "wim.h"

extern Editor editor;

#define EA_TEXTSIZE 32

typedef struct EditAction
{
    int type;
    int row;
    int col;
    int textLen;
    char text[EA_TEXTSIZE];
} EditAction;

EditAction *actionStack;

void UndoStackInit()
{
    actionStack = List(EditAction, 64);
}

void UndoStackFree()
{
    ListFree(actionStack);
}

void Undo()
{
    if (len(actionStack) == 0)
        return;

    EditAction *action = pop(actionStack);

    switch (action->type)
    {
    case A_DELETE:
        CursorSetPos(action->col, action->row, false);
        BufferWrite(action->text, action->textLen);
        break;
    }
}

void Redo()
{

}

void AppendEditAction(Action type, int row, int col, char *text)
{
    if (len(actionStack) >= 128-1)
    {
        LogError("Undo stack limit exceeded");
        return;
    }

    EditAction action;
    action.type = type;
    action.row = row;
    action.col = col;

    if (text != NULL)
    {
        strncpy_s(action.text, EA_TEXTSIZE, text, EA_TEXTSIZE);
        action.textLen = strlen(text);

        Log("Action:");
        Log(text);
    }

    append(actionStack, &action);
}