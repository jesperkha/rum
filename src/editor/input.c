#include "wim.h"

extern Editor editor;

Status HandleInsertMode(InputInfo *info)
{
    if (info->ctrlDown)
    {
        switch (info->asciiChar + 96) // Why this value?
        {
        case 'q':
            return RETURN_ERROR; // Exit

        case 'u':
            Undo();
            break;

        case 'r':
            break;

        case 'c':
            PromptCommand(NULL);
            break;

        case 'o':
            PromptCommand("open");
            break;

        case 'n':
            EditorOpenFile("");
            break;

        case 's':
            EditorSaveFile();
            break;

        case 'x':
            TypingDeleteLine();
            break;

        default:
            goto normal_input;
        }

        return RETURN_SUCCESS;
    }

normal_input:
    switch (info->keyCode)
    {
    case K_ESCAPE:
        return RETURN_ERROR; // Exit

    case K_PAGEDOWN:
        // BufferScrollDown(&buffer);
        break;

    case K_PAGEUP:
        // BufferScrollUp(&buffer);
        break;

    case K_BACKSPACE:
        TypingBackspace();
        break;

    case K_DELETE:
        TypingDelete();
        break;

    case K_ENTER:
        TypingNewline();
        break;

    case K_TAB:
        TypingInsertTab();
        break;

    case K_ARROW_UP:
        CursorMove(curBuffer, 0, -1);
        break;

    case K_ARROW_DOWN:
        CursorMove(curBuffer, 0, 1);
        break;

    case K_ARROW_LEFT:
        CursorMove(curBuffer, -1, 0);
        break;

    case K_ARROW_RIGHT:
        CursorMove(curBuffer, 1, 0);
        break;

    default:
        TypingWriteChar(info->asciiChar);
    }

    return RETURN_SUCCESS;
}