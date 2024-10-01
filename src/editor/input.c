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
        EditorPromptTabSwap();
        break;

    case 'c':
    {
        if (editor.mode != MODE_EXPLORE)
            EditorSetMode(MODE_EDIT);
        break;
    }

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
        FindPrompt();
        break;

    case 'm': // Change to o after
        EditorOpenFileExplorer();
        break;

    default:
        return false;
    }

    return true;
}

Error HandleInsertMode(InputInfo *info)
{
    switch (info->keyCode)
    {
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

    return NIL;
}

// Just basic vim movement, no editing controls
static void handleVimMovementKeys(InputInfo *info)
{
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

    case 'g':
        CursorSetPos(curBuffer, 0, 0, false);
        break;

    case 'G':
        CursorSetPos(curBuffer, 0, curBuffer->numLines - 1, false);
        break;
    }
}

typedef enum Sate
{
    S_NONE,
    S_FIND,
    S_DELETE,
    S_DELETE_INSERT,
} State;

typedef struct EditState
{
    State state;
    int direction; // 1 for right/down, -1 for left/up

    // Keys in the order they are pressed.
    // Flushed when command finishes or fails.
    char keys[8];
} EditState;

Error HandleVimMode(InputInfo *info)
{
    static EditState s = {0};
    s.keys[0] = info->asciiChar;

    if (s.state == S_FIND)
    {
        if (isChar(s.keys[0]))
        {
            s.keys[1] = s.keys[0];
            CursorSetPos(curBuffer, FindNextChar(s.keys[1], s.direction == -1), curRow, false);
            s.state = S_NONE;
            return NIL;
        }
    }

    if (s.state == S_DELETE)
    {
        if (s.keys[0] == 'd')
            TypingDeleteLine();
        else if (s.keys[0] == 'w')
        {
            int count = FindNextWordBegin() - curCol;
            if (count == 0)
                count = curLine.length - curCol;
            TypingDeleteMany(count);
        }
        else if (s.keys[0] == 'b')
        {
            int count = curCol - FindPrevWordBegin();
            if (count == 0)
                count = curCol;
            TypingBackspaceMany(count);
        }
        s.state = S_NONE;
        return NIL;
    }

    if (s.state == S_DELETE_INSERT)
    {
        if (s.keys[0] == 'c')
            TypingClearLine();
        else if (s.keys[0] == 'w')
        {
            int count = FindNextWordBegin() - curCol;
            if (count == 0)
                count = curLine.length - curCol;
            TypingDeleteMany(count);
        }
        else if (s.keys[0] == 'b')
        {
            int count = curCol - FindPrevWordBegin();
            if (count == 0)
                count = curCol;
            TypingBackspaceMany(count);
        }
        s.state = S_NONE;
        EditorSetMode(MODE_INSERT);
        return NIL;
    }

    handleVimMovementKeys(info);

    switch (info->asciiChar)
    {
    case 'u':
        Undo();
        break;

    case ':':
        PromptCommand(NULL);
        break;

    case 'f':
        s.state = S_FIND,
        s.direction = 1;
        break;

    case 'F':
        s.state = S_FIND,
        s.direction = -1;
        break;

    case ';':
        if (s.keys[1] != 0)
            CursorSetPos(curBuffer, FindNextChar(s.keys[1], s.direction == -1), curRow, false);
        break;

    case ',':
        if (s.keys[1] != 0)
            CursorSetPos(curBuffer, FindNextChar(s.keys[1], s.direction == 1), curRow, false);
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
        s.state = S_DELETE;
        break;

    case 'c':
        s.state = S_DELETE_INSERT;
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
        TypingCommentOutLine();
        break;

    case 'v':
        EditorSetMode(MODE_VISUAL);
        break;

    case 'V':
        EditorSetMode(MODE_VISUAL_LINE);
        break;

    case 'p':
        PasteFromClipboard();
        break;

    case 'P':
        CursorMove(curBuffer, 999, -1);
        TypingNewline();
        PasteFromClipboard();
        UndoJoin(2);
        break;

    default:
        break;
    }

    return NIL;
}

Error HandleVisualMode(InputInfo *info)
{
    handleVimMovementKeys(info);

    switch (info->asciiChar)
    {
    case 'd':
        TypingDeleteMarked();
        EditorSetMode(MODE_EDIT);
        break;

    case 'c':
        TypingDeleteMarked();
        EditorSetMode(MODE_INSERT);
        break;

    case ' ':
    {
        CursorPos from, to;
        BufferOrderHighlightPoints(curBuffer, &from, &to);
        TypingCommentOutLines(from.row, to.row);
        break;
    }

    case 'y':
        CopyToClipboard();
        break;

    default:
        break;
    }

    curBuffer->hlB.col = curCol;
    curBuffer->hlB.row = curRow;
    return NIL;
}

Error HandleVisualLineMode(InputInfo *info)
{
    handleVimMovementKeys(info);

    switch (info->asciiChar)
    {
    case 'y':
        CopyToClipboard();
        break;

    case 'd':
        TypingDeleteMarked();
        EditorSetMode(MODE_EDIT);
        break;

    case 'c':
        TypingDeleteMarked();
        EditorSetMode(MODE_INSERT);
        break;

    case ' ':
    {
        CursorPos from, to;
        BufferOrderHighlightPoints(curBuffer, &from, &to);
        TypingCommentOutLines(from.row, to.row);
        break;
    }

    default:
        break;
    }

    CursorPos *a = &curBuffer->hlA;
    CursorPos *b = &curBuffer->hlB;

    b->row = curRow;

    if (b->row >= a->row)
    {
        a->col = 0;
        b->col = max(curLine.length - 1, 0);
    }
    else
    {
        a->col = max(curBuffer->lines[a->row].length - 1, 0);
        b->col = 0;
    }

    return NIL;
}

Error HandleExploreMode(InputInfo *info)
{
    handleVimMovementKeys(info);

    switch (info->asciiChar)
    {
    case ':':
        PromptCommand(NULL);
        break;

    case ' ':
    {
        if (!curLine.isPath)
            break;

        char *path = BufferGetLinePath(curBuffer, &curLine);
        if (curLine.isDir)
            EditorOpenFileExplorerEx(path);
        else
            EditorOpenFile(path);
        break;
    }

    case 'b':
        EditorOpenFileExplorerEx("..");
        break;

    default:
        break;
    }

    return NIL;
}