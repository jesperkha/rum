// The editor is the main component of rum, including functionality for file IO, event handlers,
// syntax and highlight controls, configuration and more. The global editor instance is declared
// here and used by the entire core module.

#include "rum.h"

Editor editor = {0}; // Global editor instance used in core module
Colors colors = {0}; // Global constant color palette loaded from theme.json
Config config = {0}; // Global constant config loaded from config.json

char _renderBuffer[RENDER_BUFFER_SIZE];

// Populates editor global struct and creates empty file buffer. Exits on error.
void EditorInit(CmdOptions options)
{
    LogCreate;

    // IMPORTANT: Order matters
    // 1. Create buffer before loading file from options
    // 2. Set CSB and renderbuffer before any rendering
    //    Note: will blame heapFree if handles are invalid

    system("color"); // Turn on escape code output

    editor.hstdin = GetStdHandle(STD_INPUT_HANDLE);
    Assert(!(editor.hstdin == INVALID_HANDLE_VALUE));

    // Buffers used for rendering
    memset(editor.padBuffer, ' ', PAD_BUFFER_SIZE);
    editor.renderBuffer = _renderBuffer;
    AssertNotNull(editor.renderBuffer);

    // Create new temp console buffer and set as active
    editor.hbuffer = CreateConsoleScreenBuffer(GENERIC_WRITE | GENERIC_READ, 0, NULL, 1, NULL);
    Assert(!(editor.hbuffer == INVALID_HANDLE_VALUE));

    // Set as active buffer and get the size of it
    SetConsoleActiveScreenBuffer(editor.hbuffer);
    TermUpdateSize(editor);

    // Flush input of possible junk and set raw input mode (0)
    FlushConsoleInputBuffer(editor.hstdin);
    SetConsoleMode(editor.hstdin, 0);

    SetConsoleTitleA(TITLE);
    ScreenWrite("\033[?12l", 6); // Turn off cursor blinking

    // Set up editor and handle config/options
    EditorNewBuffer();
    EditorSetActiveBuffer(0);
    EditorSetMode(MODE_EDIT);

    if (LoadConfig(&config) != NIL)
        ErrorExit("Failed to load config file");

    if (LoadTheme(config.theme, &colors) != NIL)
        ErrorExit("Failed to load default theme");

    if (options.hasFile && EditorOpenFile(options.filename) != NIL)
        ErrorExitf("File '%s' not found", options.filename);

    config.rawMode = options.rawMode;

    SetError(NULL);
    Render();
    Log("Init finished");
}

// Asks user if they want to exit without saving. Writes file if answered yes.
static void promptFileNotSaved(Buffer *b)
{
    char prompt[512];
    strcpy(prompt, "Save file before closing? ");
    strcat(prompt, b->filepath);

    if (b->isFile && b->dirty)
        if (UiPromptYesNo(prompt, true) == UI_YES)
            EditorSaveFile();
}

// Replaces current buffer with b. Sets the id of curbuf to b. Prompt file not saved.
static void replaceCurrentBuffer(Buffer *b)
{
    promptFileNotSaved(curBuffer);
    int id = curBuffer->id;
    BufferFree(curBuffer);
    curBuffer = b;
    curBuffer->id = id;
    EditorSetActiveBuffer(id);
}

void EditorFree()
{
    for (int i = 0; i < editor.numBuffers; i++)
    {
        promptFileNotSaved(editor.buffers[i]);
        BufferFree(editor.buffers[i]);
    }

    CloseHandle(editor.hbuffer);
    Log("Editor free successful");
}

Error EditorReadInput(InputInfo *info)
{
    INPUT_RECORD record;
    DWORD read;
    if (!ReadConsoleInputA(editor.hstdin, &record, 1, &read) || read == 0)
    {
        Errorf("Failed to read from input handle. WinError: %d", (int)GetLastError());
        return ERR_INPUT_READ_FAIL;
    }

    info->eventType = INPUT_UNKNOWN;

    if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown)
    {
        KEY_EVENT_RECORD event = record.Event.KeyEvent;
        info->eventType = INPUT_KEYDOWN;
        info->keyCode = event.wVirtualKeyCode;
        info->asciiChar = event.uChar.AsciiChar;
        info->ctrlDown = event.dwControlKeyState & LEFT_CTRL_PRESSED;
    }
    else if (record.EventType == WINDOW_BUFFER_SIZE_EVENT)
        info->eventType = INPUT_WINDOW_RESIZE;

    return NIL;
}

// Waits for input and takes action for insert mode.
Error EditorHandleInput()
{
    InputInfo info;

    Error err = EditorReadInput(&info);
    if (err != NIL)
        return err;

    if (info.eventType == INPUT_WINDOW_RESIZE)
    {
        TermUpdateSize();
        return NIL;
    }

    if (info.eventType == INPUT_KEYDOWN)
    {
        if (info.ctrlDown && HandleCtrlInputs(&info))
            return NIL;

        Error s;
        switch (editor.mode)
        {
        case MODE_INSERT:
        {
            s = HandleInsertMode(&info);
            break;
        }

        case MODE_EDIT:
        {
            s = HandleVimMode(&info);
            break;
        }

        case MODE_VISUAL:
        {
            s = HandleVisualMode(&info);
            break;
        }

        case MODE_VISUAL_LINE:
        {
            s = HandleVisualLineMode(&info);
            break;
        }

        case MODE_EXPLORE:
        {
            s = HandleExploreMode(&info);
            break;
        }

        default:
            Panicf("Unhandled input mode %d", editor.mode);
            break;
        }

        // Make sure edit or visual mode doesnt go beyond line length
        if (editor.mode != MODE_INSERT)
        {
            capValue(curBuffer->cursor.col, max(curLine.length - 1, 0));
            capValue(curBuffer->hlB.col, max(curLine.length - 1, 0));
        }

        return s;
    }

    return NIL; // Unhandled event
}

// Loads file into current buffer. Filepath must either be an absolute path
// or name of a file in the same directory as working directory.
Error EditorOpenFile(char *filepath)
{
    if (filepath == NULL || strlen(filepath) == 0)
    {
        // Empty buffer
        replaceCurrentBuffer(BufferNew());
        return NIL;
    }

    int size;
    char *buf = IoReadFile(filepath, &size);
    if (buf == NULL)
        return ERR_FILE_NOT_FOUND;

    // Change active buffer
    Buffer *newBuf = BufferLoadFile(filepath, buf, size);

    replaceCurrentBuffer(newBuf);
    return NIL;
}

// Writes content of buffer to filepath. Always truncates file.
Error EditorSaveFile()
{
    if (!BufferSaveFile(curBuffer))
        return ERR_FILE_SAVE_FAIL;

    curBuffer->dirty = false;
    return NIL;
}

void EditorPromptCommand()
{
    SetError(NULL);

    char cmdBuf[MAX_SEARCH];
    int cmdLen = 0;

    UiStatus status;
    while ((status = UiInputBox("Command", cmdBuf, &cmdLen, MAX_SEARCH)) == UI_CONTINUE)
        ;

    if (status != UI_OK || cmdLen == 0)
        return;

    // Split command string by spaces
    char *args[MAX_ARGS];
    int argc = 0;
    {
        char *p = strtok(cmdBuf, " ");
        while (p != NULL && argc < MAX_ARGS)
        {
            args[argc++] = p;
            p = strtok(NULL, " ");
        }
    }

#define IS_COMMAND(c, body)  \
    if (!strcmp(args[0], c)) \
    {                        \
        {                    \
            body;            \
        }                    \
        return;              \
    }

    IS_COMMAND("o", {
        if (argc == 1)
            EditorOpenFile(NULL);
        else if (argc > 2)
            SetError("usage: o [filepath?]");
        else if (EditorOpenFile(args[1]) != NIL)
            SetError("file not found");
    })

    IS_COMMAND("n", {
        if (argc == 1)
            EditorOpenFile(NULL);
        else if (argc > 2)
            SetError("usage: n [filepath?]");
        else
        {
            if (IoFileExists(args[1]))
            {
                SetError("file already exists");
                return;
            }
            EditorOpenFile(NULL);
            BufferSetFilename(curBuffer, args[1]);
        }
    })

    IS_COMMAND("q", {
        EditorFree();
        ExitProcess(0);
    })

    IS_COMMAND("w", {
        EditorSaveFile();
    })

    IS_COMMAND("help", {
        EditorShowHelp();
    })

    IS_COMMAND("noh", {
        BufferUnmarkAll(curBuffer);
    })

    IS_COMMAND("theme", {
        if (argc != 2)
        {
            SetError("usage: theme [name]");
            return;
        }

        if (LoadTheme(args[1], &colors) != NIL)
            SetError("theme not found");
    })

    IS_COMMAND("spaces", {
        curBuffer->useTabs = false;
    })

    IS_COMMAND("tabs", {
        curBuffer->useTabs = true;
    })

    IS_COMMAND("hl", {
        if (argc > 1)
        {
            if (!BufferSetFileType(curBuffer, args[1]))
                SetError("Unknown file type");
        }
        else
            SetError("usage: hl [file extension]");
    })

    // Invalid command name
    SetError("unknown command");
}

#define isVisual(m) ((m) == MODE_VISUAL || (m) == MODE_VISUAL_LINE)

void EditorSetMode(InputMode mode)
{
    if (editor.mode == MODE_INSERT && mode == MODE_EDIT)
        CursorMove(curBuffer, -1, 0);

    if (isVisual(editor.mode))
    {
        CursorPos from, to;
        BufferOrderHighlightPoints(curBuffer, &from, &to);
        CursorSetPos(curBuffer, from.col, from.row, false);
    }

    // Toggle highlighting (purely visual)
    curBuffer->showHighlight = isVisual(mode);
    curBuffer->cursor.visible = !isVisual(mode);

    if (isVisual(mode))
    {
        if (mode == MODE_VISUAL_LINE)
        {
            curBuffer->hlA = (CursorPos){curRow, 0};
            curBuffer->hlB = (CursorPos){curRow, max(curLine.length - 1, 0)};
        }
        else
        {
            curBuffer->hlA = (CursorPos){curRow, curCol};
            curBuffer->hlB = (CursorPos){curRow, curCol};
        }
    }

    editor.mode = mode;
}

extern char HELP_TEXT[];

void EditorShowHelp()
{
    UiTextbox(HELP_TEXT);
}

int EditorNewBuffer()
{
    if (editor.numBuffers == EDITOR_BUFFER_CAP)
        Error("Maximum number of buffers exceeded");

    Buffer *b = BufferNew();
    b->id = editor.numBuffers;
    editor.buffers[editor.numBuffers] = b;
    editor.numBuffers++;
    return editor.numBuffers - 1;
}

void EditorSplitBuffers()
{
    if (editor.splitBuffers)
        return;

    editor.splitBuffers = true;
    if (editor.rightBuffer == editor.leftBuffer)
        editor.rightBuffer = EditorNewBuffer();

    editor.activeBuffer = editor.rightBuffer;
}

void EditorUnsplitBuffers()
{
    if (!editor.splitBuffers)
        return;

    editor.splitBuffers = false;
    editor.activeBuffer = editor.leftBuffer;
}

void EditorSetActiveBuffer(int idx)
{
    EditorSetMode(MODE_EDIT); // This is also a hack to reset visual mode when switching buffers
    if (editor.buffers[idx]->isDir)
        EditorSetMode(MODE_EXPLORE);
    editor.activeBuffer = idx;
}

void EditorSwapActiveBuffer(int idx)
{
    if (editor.splitBuffers && editor.rightBuffer == editor.activeBuffer)
        editor.rightBuffer = idx;
    else
        editor.leftBuffer = idx;
    EditorSetActiveBuffer(idx);
}

void EditorCloseBuffer(int idx)
{
    if (editor.numBuffers == 1)
    {
        EditorOpenFile("");
        return;
    }

    BufferFree(editor.buffers[idx]);
    editor.numBuffers--;

    // Move buffers on right side of idx one left
    size_t count = (editor.numBuffers - idx) * sizeof(Buffer *);
    memmove(editor.buffers + idx, editor.buffers + idx + 1, count);

    // Update id for each of the buffers
    for (int i = idx; i < editor.numBuffers; i++)
        editor.buffers[i]->id--;

    // Current working hack
    editor.leftBuffer = clamp(0, editor.numBuffers - 1, editor.leftBuffer);
    editor.rightBuffer = clamp(0, editor.numBuffers - 1, editor.rightBuffer);
    editor.activeBuffer = clamp(0, editor.numBuffers - 1, editor.activeBuffer);
}

void EditorPromptTabSwap()
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

void EditorOpenFileExplorer()
{
    EditorOpenFileExplorerEx(".");
}

void EditorOpenFileExplorerEx(char *directory)
{
    SetCurrentDirectoryA(directory);

    char *helpText = "<space> go into   <b> go back";

    Buffer *exBuf = BufferNew();
    exBuf->exPaths = StrArrayNew(KB(0.5));

    char fullPath[PATH_MAX + 2];
    GetCurrentDirectoryA(PATH_MAX, fullPath);
    strcat(fullPath, "/*");

    // Itrerate over files in directory and write to buffer
    WIN32_FIND_DATAA file;
    HANDLE hFind = FindFirstFileA(fullPath, &file);
    char lineFormatString[1024];
    int numDirs = 0; // Keeping track of dir count for sorting

    do
    {
        bool isDir = file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

        UINT64 fileSize = (UINT64)file.nFileSizeLow | ((UINT64)file.nFileSizeHigh << 32);
        char fileSizeS[64];
        StrNumberToReadable(fileSize, fileSizeS);

        // Get modification date as dd.mm.yyyy
        char date[64];
        SYSTEMTIME sysTime;
        FileTimeToSystemTime(&file.ftLastWriteTime, &sysTime);
        GetDateFormatA(LOCALE_CUSTOM_DEFAULT, 0, &sysTime, NULL, date, 64);

        char *filename = file.cFileName;
        int filenameLen = strlen(file.cFileName);
        int lineLen = sprintf(lineFormatString, "%s %s %s", fileSizeS, date, filename);
        int row = isDir ? (++numDirs) : -1; // Sorting by directories first

        Line *line = BufferInsertLineEx(exBuf, row, lineFormatString, lineLen);
        line->exPathId = StrArraySet(&exBuf->exPaths, filename, filenameLen);
        line->isPath = true;
        line->isDir = isDir;

    } while (FindNextFileA(hFind, &file));
    FindClose(hFind);

    BufferInsertLineEx(exBuf, 0, helpText, strlen(helpText));
    BufferInsertLine(exBuf, 0);

    // Add path to buffer
    {
        int fullPathLen = strlen(fullPath);
        fullPath[fullPathLen - 2] = 0; // Remove \* used for search
        BufferInsertLineEx(exBuf, 0, fullPath, fullPathLen - 2);
    }

    // Configure buffer
    strcpy(exBuf->filepath, StrGetShortPath(fullPath)); // Do not use fullPath after this
    exBuf->isDir = true;
    exBuf->readOnly = true;

    replaceCurrentBuffer(exBuf);
    EditorSetMode(MODE_EXPLORE);

    // Set cursor at first dir for convenience
    CursorSetPos(exBuf, 999, 4, false);
    BufferScroll(exBuf);
}