#include "wim.h"

extern Editor editor;

// Returns true if a key action was triggered
static bool handleCtrlInputs(InputInfo *info)
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
        return false;
    }

    return true;
}

Status HandleInsertMode(InputInfo *info)
{
    if (info->ctrlDown && handleCtrlInputs(info))
        return RETURN_SUCCESS;

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

typedef enum Sate
{
    S_NONE,
    S_FIND,
} State;

Status HandleVimMode(InputInfo *info)
{
    static State state = S_NONE;
    static char char2 = 0;
    static int findDir = 1; // 1 or -1

    if (info->ctrlDown && handleCtrlInputs(info))
        return RETURN_SUCCESS;

    switch (info->keyCode)
    {
    case K_ESCAPE:
        return RETURN_ERROR; // Exit
    default:
        break;
    }

    if (state != S_NONE)
    {
        switch (state)
        {
        case S_FIND:
            if (!isChar(info->asciiChar))
                break;
            char2 = info->asciiChar;
            CursorSetPos(curBuffer, FindNextChar(char2, findDir == -1), curRow, false);
            state = S_NONE;
            break;

        default:
            Panic("Unhandled input state");
            break;
        }

        return RETURN_SUCCESS;
    }

    switch (info->asciiChar)
    {
    case 'u':
        Undo();
        break;

    case 'f':
        state = S_FIND,
        findDir = 1;
        break;

    case 'F':
        state = S_FIND,
        findDir = -1;
        break;

    case ';':
        if (char2 != 0)
            CursorSetPos(curBuffer, FindNextChar(char2, findDir == -1), curRow, false);
        break;

    case ',':
        if (char2 != 0)
            CursorSetPos(curBuffer, FindNextChar(char2, findDir == 1), curRow, false);
        break;

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
        CursorMove(curBuffer, 999, 0);
        TypingNewline();
        EditorSetMode(MODE_INSERT);
        break;

    case 'O':
        CursorMove(curBuffer, 999, -1);
        TypingNewline();
        EditorSetMode(MODE_INSERT);
        break;

    case 'x':
    {
        if (curCol < curLine.length)
            TypingDelete();
        break;
    }

    case 'D':
        TypingDeleteLine();
        CursorMove(curBuffer, 0, 0);
        break;

    default:
        break;
    }

    return RETURN_SUCCESS;
}