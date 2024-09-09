#include "rum.h"

extern Editor editor;

bool HandleCtrlInputs(InputInfo *info)
{
    switch (info->asciiChar + 96) // Why this value?
    {
    case 'q':
        EditorFree();
        ExitProcess(0);
        break;

    case 'y':
        if (!editor.splitBuffers)
            EditorSplitBuffers();
        else
            EditorUnsplitBuffers();
        break;

    case 'h':
        EditorSetActiveBuffer(editor.leftBuffer);
        break;

    case 'l':
        EditorSetActiveBuffer(editor.rightBuffer);
        break;

    case 'z':
        Undo();
        break;

    case 'e':
    {
        char *empty = "[empty]";
        char *items[EDITOR_BUFFER_CAP];
        for (int i = 0; i < editor.numBuffers; i++)
        {
            Buffer *b = editor.buffers[i];
            items[i] = b->isFile ? b->filepath : empty;
        }
        UiResult res = UiPromptListEx(items, editor.numBuffers, "Switch buffer:", editor.activeBuffer);
        if (res.status == UI_OK)
            EditorSwapActiveBuffer(res.choice);
    }
    break;

    case 'c':
        EditorSetMode(MODE_EDIT);
        break;

    case 'o':
        PromptCommand("open");
        break;

    case 'n':
        EditorOpenFile("");
        break;

    case 't':
        EditorSwapActiveBuffer(EditorNewBuffer());
        break;

    case 'w':
        EditorCloseBuffer(editor.activeBuffer);
        break;

    case 's':
        EditorSaveFile();
        break;

    case 'x':
        TypingDeleteLine();
        break;

    case 'f':
        UiResult res = UiGetTextInput("Find: ", MAX_SEARCH);
        strncpy(curBuffer->search, res.buffer, res.length);
        curBuffer->searchLen = res.length;
        CursorPos pos = FindNext(res.buffer, res.length);
        CursorSetPos(curBuffer, pos.col, pos.row, false);
        BufferCenterView(curBuffer);
        UiFreeResult(res);
        break;

    default:
        return false;
    }

    return true;
}

Status HandleInsertMode(InputInfo *info)
{
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
    S_DELETE,
    S_DELETE_INSERT,
} State;

Status HandleVimMode(InputInfo *info)
{
    static State state = S_NONE;
    static char char2 = 0;
    static int findDir = 1; // 1 or -1

    if (info->keyCode == K_ESCAPE)
        return RETURN_ERROR; // Exit

    if (state != S_NONE)
    {
        switch (state)
        {
        case S_FIND:
        {
            if (!isChar(info->asciiChar))
                break;
            char2 = info->asciiChar;
            CursorSetPos(curBuffer, FindNextChar(char2, findDir == -1), curRow, false);
            state = S_NONE;
            break;
        }

        case S_DELETE:
        {
            char c = info->asciiChar;
            if (c == 'd')
                TypingDeleteLine();
            else if (c == 'w')
            {
                int count = FindNextWordBegin() - curCol;
                if (count == 0)
                    count = curLine.length - curCol;
                TypingDeleteMany(count);
            }
            else if (c == 'b')
            {
                int count = curCol - FindPrevWordBegin();
                if (count == 0)
                    count = curCol;
                TypingBackspaceMany(count);
            }
            state = S_NONE;
            break;
        }

        case S_DELETE_INSERT:
        {
            char c = info->asciiChar;
            if (c == 'c')
                TypingClearLine();
            else if (c == 'w')
            {
                int count = FindNextWordBegin() - curCol;
                if (count == 0)
                    count = curLine.length - curCol;
                TypingDeleteMany(count);
            }
            else if (c == 'b')
            {
                int count = curCol - FindPrevWordBegin();
                if (count == 0)
                    count = curCol;
                TypingBackspaceMany(count);
            }
            state = S_NONE;
            EditorSetMode(MODE_INSERT);
            break;
        }

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

    case ':':
        PromptCommand(NULL);
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

    case 'J':
        CursorSetPos(curBuffer, curCol, FindNextBlankLine(), false);
        break;

    case 'k':
        CursorMove(curBuffer, 0, -1);
        break;

    case 'K':
        CursorSetPos(curBuffer, curCol, FindPrevBlankLine(), false);
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
        CursorSetPos(curBuffer, FindLineBegin(), curRow, false);
        TypingNewline();
        CursorMove(curBuffer, 999, -1);
        EditorSetMode(MODE_INSERT);
        break;

    case 'x':
        if (curCol < curLine.length)
            TypingDelete();
        break;

    case 's':
        if (curCol < curLine.length)
            TypingDelete();
        EditorSetMode(MODE_INSERT);
        break;

    case 'S':
        TypingClearLine();
        EditorSetMode(MODE_INSERT);
        break;

    case 'd':
        state = S_DELETE;
        break;

    case 'c':
        state = S_DELETE_INSERT;
        break;

    case 'D':
        TypingDeleteMany(curLine.length - curCol);
        break;

    case 'C':
        TypingDeleteMany(curLine.length - curCol);
        EditorSetMode(MODE_INSERT);
        break;

    case 'n':
        if (curBuffer->searchLen != 0)
        {
            CursorPos pos = FindNext(curBuffer->search, curBuffer->searchLen);
            CursorSetPos(curBuffer, pos.col, pos.row, false);
        }
        break;

    case 'N':
        if (curBuffer->searchLen != 0)
        {
            CursorPos pos = FindPrev(curBuffer->search, curBuffer->searchLen);
            CursorSetPos(curBuffer, pos.col, pos.row, false);
        }
        break;

    case ' ': // Debug
        BufferCenterView(curBuffer);
        break;

    case 'g':
        CursorSetPos(curBuffer, 0, 0, false);
        break;

    case 'G':
        CursorSetPos(curBuffer, 0, curBuffer->numLines - 1, false);
        break;

    case 'v':
        EditorSetMode(MODE_VISUAL);
        break;

    default:
        break;
    }

    return RETURN_SUCCESS;
}

Status HandleVisualMode(InputInfo *info)
{
    if (info->keyCode == K_ESCAPE)
        return RETURN_ERROR; // Exit

    switch (info->asciiChar)
    {
    case 'j':
        CursorMove(curBuffer, 0, 1);
        break;

    case 'J':
        CursorSetPos(curBuffer, curCol, FindNextBlankLine(), false);
        break;

    case 'k':
        CursorMove(curBuffer, 0, -1);
        break;

    case 'K':
        CursorSetPos(curBuffer, curCol, FindPrevBlankLine(), false);
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

    default:
        break;
    }

    curBuffer->hlB.col = curCol;
    curBuffer->hlB.row = curRow;
    return RETURN_SUCCESS;
}