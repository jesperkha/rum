#include "rum.h"

extern Editor editor;

bool HandleCtrlInputs(InputInfo *info)
{
    if (!info->ctrlDown)
        return false;

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

        // case 'h':
        //     EditorSetActiveBuffer(editor.leftBuffer);
        //     break;

        // case 'l':
        //     EditorSetActiveBuffer(editor.rightBuffer);
        //     break;

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

    case 'o':
        EditorOpenFileExplorer();
        break;

    case 'h':
        EditorShowHelp();
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
    case K_ESCAPE:
        EditorSetMode(MODE_EDIT);
        break;

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

    switch (info->keyCode)
    {
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
        break;
    }
}

static char getNextInputChar()
{
    InputInfo info;

    while (true)
    {
        Error err = EditorReadInput(&info);
        if (err != NIL)
            Panic("Failed to read input");

        if (info.eventType == INPUT_WINDOW_RESIZE)
            TermUpdateSize();

        if (info.eventType == INPUT_KEYDOWN)
        {
            if (HandleCtrlInputs(&info))
                return 0;
            if (isChar(info.asciiChar))
                break;
        }
    }

    return info.asciiChar;
}

Error HandleVimMode(InputInfo *info)
{
    static char findChar = 0;
    static bool backwards = false;
    char key1 = info->asciiChar;

    if (key1 == 'f')
    {
        findChar = getNextInputChar();
        backwards = false;
        CursorSetPos(curBuffer, FindNextChar(findChar, backwards), curRow, false);
        return NIL;
    }
    else if (key1 == 'F')
    {
        findChar = getNextInputChar();
        backwards = true;
        CursorSetPos(curBuffer, FindNextChar(findChar, backwards), curRow, false);
        return NIL;
    }
    else if (key1 == 'd')
    {
        char key2 = getNextInputChar();

        if (key2 == 'd')
            TypingDeleteLine();
        else if (key2 == 'w')
        {
            int count = FindNextWordBegin() - curCol;
            if (count == 0)
                count = curLine.length - curCol;
            TypingDeleteMany(count);
        }
        else if (key2 == 'b')
        {
            int count = curCol - FindPrevWordBegin();
            if (count == 0)
                count = curCol;
            TypingBackspaceMany(count);
        }

        return NIL;
    }
    else if (key1 == 'c')
    {
        char key2 = getNextInputChar();

        if (key2 == 'c')
            TypingClearLine();
        else if (key2 == 'w')
        {
            int count = FindNextWordBegin() - curCol;
            if (count == 0)
                count = curLine.length - curCol;
            TypingDeleteMany(count);
        }
        else if (key2 == 'b')
        {
            int count = curCol - FindPrevWordBegin();
            if (count == 0)
                count = curCol;
            TypingBackspaceMany(count);
        }

        EditorSetMode(MODE_INSERT);
        return NIL;
    }
    else if (key1 == ' ')
    {
        char key2 = getNextInputChar();

        if (key2 == 'c')
            TypingCommentOutLine();
        else if (key2 == 'h')
            EditorSetActiveBuffer(editor.leftBuffer);
        else if (key2 == 'l')
            EditorSetActiveBuffer(editor.rightBuffer);
        else if (key2 == 't')
            EditorPromptTabSwap();
        else if (key2 == 's')
            EditorSplitBuffers();
        else if (key2 == 'e')
            EditorOpenFileExplorer();

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

    case ';':
        if (isChar(findChar))
            CursorSetPos(curBuffer, FindNextChar(findChar, backwards), curRow, false);
        break;

    case ',':
        if (isChar(findChar))
            CursorSetPos(curBuffer, FindNextChar(findChar, !backwards), curRow, false);
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

static void selectDirectory()
{
    if (!curLine.isPath)
        return;

    char *path = BufferGetLinePath(curBuffer, &curLine);
    if (curLine.isDir)
        EditorOpenFileExplorerEx(path);
    else
        EditorOpenFile(path);
}

Error HandleExploreMode(InputInfo *info)
{
    handleVimMovementKeys(info);

    if (info->keyCode == K_ENTER)
        selectDirectory();

    switch (info->asciiChar)
    {
    case ':':
        PromptCommand(NULL);
        break;

    case ' ':
        selectDirectory();
        break;

    case 'b':
        EditorOpenFileExplorerEx("..");
        break;

    default:
        break;
    }

    return NIL;
}