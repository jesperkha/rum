#include "rum.h"

extern Editor editor;

void UndoSaveAction(Action type, char *text, int textLen)
{
    UndoSaveActionEx(type, curRow, curCol, text, textLen);
}

void UndoSaveActionEx(Action type, int row, int col, char *text, int textLen)
{
    if (len(curBuffer->undos) > 0 && type == A_WRITE && isalnum(text[0]))
    {
        // If there is no word break just append to the last undo
        EditorAction *last = &curBuffer->undos[len(curBuffer->undos) - 1];
        if (last->type == A_WRITE && row == last->row && col == last->col + last->textLen)
        {
            if (last->textLen + textLen < EDITOR_ACTION_BUFSIZE)
            {
                strncat(last->text, text, textLen);
                last->textLen += textLen;
                return;
            }
        }
    }

    if (len(curBuffer->undos) > 0 && type == A_BACKSPACE && isalnum(text[0]))
    {
        // If there is no word break just append to the last undo
        EditorAction *last = &curBuffer->undos[len(curBuffer->undos) - 1];
        if (last->type == A_BACKSPACE && row == last->row && col == last->col - 1)
        {
            if (last->textLen + textLen < EDITOR_ACTION_BUFSIZE)
            {
                strncat(last->text, text, textLen);
                last->textLen += textLen;
                last->col -= textLen;
                return;
            }
        }
    }

    EditorAction action = {
        .type = type,
        .col = col,
        .row = row,
        .textLen = textLen,
        .noNewline = (row == 0 && curBuffer->numLines == 1),
        .isLongText = textLen > EDITOR_ACTION_BUFSIZE,
    };

    if (textLen > EDITOR_ACTION_BUFSIZE)
    {
        action.longText = MemAlloc(textLen);
        memcpy(action.longText, text, textLen);
    }
    else
        strncpy(action.text, text, textLen);

    append(curBuffer->undos, &action);
}

// Joins last n actions under same undo call.
void UndoJoin(int n)
{
    EditorAction a = {
        .type = A_JOIN,
        .numUndos = n,
    };

    append(curBuffer->undos, &a);
}

void Undo()
{
    if (len(curBuffer->undos) == 0)
        return;

    EditorAction *a = pop(curBuffer->undos);
    char *undoText = a->isLongText ? a->longText : a->text;

    switch (a->type)
    {
    case A_JOIN:
    {
        for (int i = 0; i < a->numUndos; i++)
            Undo();
    }
    break;

    case A_CURSOR:
    {
        CursorSetPos(curBuffer, a->col, a->row, false);
    }
    break;

    case A_WRITE:
    {
        int length = a->textLen;
        BufferDeleteEx(curBuffer, a->row, a->col + length, length);
        CursorSetPos(curBuffer, a->col, a->row, false);
    }
    break;

    case A_DELETE:
    {
        BufferWriteEx(curBuffer, a->row, a->col, undoText, a->textLen);
        CursorSetPos(curBuffer, a->col, a->row, false);
    }
    break;

    case A_DELETE_BACK:
    {
        BufferWriteEx(curBuffer, a->row, a->col, undoText, a->textLen);
        CursorSetPos(curBuffer, a->col + a->textLen, a->row, false);
    }
    break;

    case A_BACKSPACE:
    {
        BufferWriteEx(curBuffer, a->row, a->col, strrev(undoText), a->textLen);
        CursorSetPos(curBuffer, a->col + a->textLen, a->row, false);
    }
    break;

    case A_INSERT_LINE:
    {
        BufferOverWriteEx(curBuffer, a->row - 1, 0, undoText, a->textLen);
        BufferDeleteLine(curBuffer, a->row);
        CursorSetPos(curBuffer, a->col, a->row - 1, false);
    }
    break;

    case A_DELETE_LINE:
    {
        if (a->noNewline)
            BufferWrite(curBuffer, undoText, a->textLen);
        else
            BufferInsertLineEx(curBuffer, a->row, undoText, a->textLen);
        CursorSetPos(curBuffer, a->col, a->row, false);
    }
    break;

    default:
        Errorf("Undo not implemented for action: %d", a->type);
    }

    if (a->isLongText)
        MemFree(a->longText);
}
