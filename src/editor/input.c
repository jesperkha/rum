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
            // PromptCommand(NULL);
            EditorSetMode(MODE_VIM);
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

Status HandleVimMode(InputInfo *info)
{
    switch (info->keyCode)
    {
    case K_ESCAPE:
        return RETURN_ERROR; // Exit
    default:
        break;
    }

    switch (info->asciiChar)
    {
    case 'j':
        CursorMove(curBuffer, 0, 1);
        break;

    case 'k':
        CursorMove(curBuffer, 0, -1);
        break;

    case 'h':
        CursorMove(curBuffer, -1, 0);
        break;

    case 'H':
        CursorSetPos(curBuffer, FindLineBegin(), curRow, false);
        break;

    case 'L':
        CursorSetPos(curBuffer, FindLineEnd(), curRow, false);
        break;

    case 'l':
        CursorMove(curBuffer, 1, 0);
        break;

    case 'w':
        CursorSetPos(curBuffer, FindNextWordBegin(), curRow, false);
        break;

    case 'b':
        CursorSetPos(curBuffer, FindPrevWordBegin(), curRow, false);
        break;

    case 'i':
        EditorSetMode(MODE_INSERT);
        break;

    case 'I':
        CursorSetPos(curBuffer, FindLineBegin(), curRow, false);
        EditorSetMode(MODE_INSERT);
        break;

    case 'a':
        CursorMove(curBuffer, 1, 0);
        EditorSetMode(MODE_INSERT);
        break;

    case 'A':
        CursorSetPos(curBuffer, FindLineEnd() + 1, curRow, false);
        EditorSetMode(MODE_INSERT);
        break;

    case 'o':
        BufferInsertLine(curBuffer, curRow + 1);
        CursorMove(curBuffer, 0, 1);
        EditorSetMode(MODE_INSERT);
        break;

    case 'O':
        BufferInsertLine(curBuffer, curRow);
        EditorSetMode(MODE_INSERT);
        CursorMove(curBuffer, 0, 0);
        break;

    case 'x':
    {
        if (curCol >= curLine.length)
            break;
        CursorMove(curBuffer, 1, 0);
        BufferDelete(curBuffer, 1);
        CursorMove(curBuffer, -1, 0);
        break;
    }

    default:
        break;
    }

    return RETURN_SUCCESS;
}