// The editor is the main component of rum, including functionality for file IO, event handlers,
// syntax and highlight controls, configuration and more. The editor global instance is declared
// here and used by the entire core module.

#include "rum.h"

Editor editor; // Global editor instance used in core module
Colors colors; // Global constant color palette loaded from theme.json
Config config; // Global constant config loaded from config.json

static void updateSize();

void error_exit(char *msg)
{
    printf("Error: %s\n", msg);
    Errorf("%s", msg);
    ExitProcess(1);
}

// Initializes stuff related to the actual terminal
void initTerm()
{
    // Create new temp buffer and set as active
    editor.hbuffer = CreateConsoleScreenBuffer(GENERIC_WRITE | GENERIC_READ, 0, NULL, 1, NULL);
    if (editor.hbuffer == INVALID_HANDLE_VALUE)
        error_exit("failed to create new console screen buffer");

    SetConsoleActiveScreenBuffer(editor.hbuffer);
    updateSize();

    COORD maxSize = GetLargestConsoleWindowSize(editor.hbuffer);
    if ((editor.renderBuffer = MemAlloc(maxSize.X * maxSize.Y * 4)) == NULL)
        error_exit("failed to allocate renderBuffer");

    editor.hstdin = GetStdHandle(STD_INPUT_HANDLE);
    if (editor.hstdin == INVALID_HANDLE_VALUE)
        error_exit("failed to get input handle");

    // 0 flag enabled 'raw' mode in terminal
    SetConsoleMode(editor.hstdin, 0);
    FlushConsoleInputBuffer(editor.hstdin);

    SetConsoleTitleA(TITLE);
    ScreenWrite("\033[?12l", 6); // Turn off cursor blinking
    SetStatus("[empty file]", NULL);
}

// Populates editor global struct and creates empty file buffer. Exits on error.
void EditorInit(CmdOptions options)
{
    system("color"); // Turn on escape code output
    LogCreate;

    editor.hbuffer = INVALID_HANDLE_VALUE;

    // IMPORTANT:
    // order here matters a lot
    // buffer must be created before a file is loaded
    // csb must be set before any call to render/ScreenWrite etc
    // win32 does not give a shit if the handle is invalid and will
    // blame literally anything else (especially HeapFree for some reason)

    editor.activeBuffer = 0;
    editor.numBuffers = 0;
    editor.leftBuffer = 0;
    editor.rightBuffer = 0;
    editor.splitBuffers = false;
    EditorNewBuffer();
    EditorSetActiveBuffer(0);

    editor.mode = MODE_EDIT;

    if (!LoadTheme("dracula", &colors))
        error_exit("Failed to load default theme");

    if (!LoadConfig(&config))
        error_exit("Failed to load config file");

    if (options.hasFile)
    {
        if (!EditorOpenFile(options.filename))
            error_exit("File not found");
    }

    config.rawMode = options.rawMode;

    memset(editor.padBuffer, ' ', MAX_PADDING);

    initTerm(); // Must be called before render
    Render();
    Log("Init");
}

void EditorFree()
{
    for (int i = 0; i < editor.numBuffers; i++)
    {
        PromptFileNotSaved(editor.buffers[i]);
        BufferFree(editor.buffers[i]);
    }

    MemFree(editor.renderBuffer);
    CloseHandle(editor.hbuffer);
    Log("Editor free successful");
}

// Hangs when waiting for input. Returns error if read failed. Writes to info.
Status EditorReadInput(InputInfo *info)
{
    INPUT_RECORD record;
    DWORD read;
    if (!ReadConsoleInputA(editor.hstdin, &record, 1, &read) || read == 0)
        return RETURN_ERROR;

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

    return RETURN_SUCCESS;
}

// Waits for input and takes action for insert mode.
Status EditorHandleInput()
{
    InputInfo info;
    if (EditorReadInput(&info) == RETURN_ERROR)
        return RETURN_ERROR;

    if (info.eventType == INPUT_WINDOW_RESIZE)
    {
        updateSize();
        Render();
        return RETURN_SUCCESS;
    }

    if (info.eventType == INPUT_KEYDOWN)
    {
        switch (editor.mode)
        {
        case MODE_INSERT:
        {
            if (!HandleInsertMode(&info))
                return RETURN_ERROR;
        }
        break;

        case MODE_EDIT:
        {
            if (!HandleVimMode(&info))
                return RETURN_ERROR;
        }
        break;

        default:
            break;
        }

        Render();
        return RETURN_SUCCESS;
    }

    return RETURN_SUCCESS; // Unknown event
}

// Loads file into current buffer. Filepath must either be an absolute path
// or name of a file in the same directory as working directory.
Status EditorOpenFile(char *filepath)
{
    if (filepath == NULL || strlen(filepath) == 0)
    {
        // Empty buffer
        EditorSetCurrentBuffer(BufferNew());
        return RETURN_SUCCESS;
    }

    PromptFileNotSaved(curBuffer);

    int size;
    char *buf = EditorReadFile(filepath, &size);
    if (buf == NULL)
        return RETURN_ERROR;

    // Change active buffer
    Buffer *newBuf = BufferLoadFile(filepath, buf, size);
    EditorSetCurrentBuffer(newBuf);

    SetStatus(filepath, NULL);

    // Load syntax for file
    LoadSyntax(newBuf, filepath);

    return RETURN_SUCCESS;
}

void EditorSetCurrentBuffer(Buffer *b)
{
    int id = curBuffer->id;
    BufferFree(curBuffer);
    curBuffer = b;
    curBuffer->id = id;
}

// Writes content of buffer to filepath. Always truncates file.
Status EditorSaveFile()
{
    if (!BufferSaveFile(curBuffer))
        return RETURN_ERROR;

    LoadSyntax(curBuffer, curBuffer->filepath);
    curBuffer->dirty = false;
    return RETURN_SUCCESS;
}

// Update editor and screen buffer size.
static void updateSize()
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(editor.hbuffer, &info);

    short bufferW = info.dwSize.X;
    short windowH = info.srWindow.Bottom - info.srWindow.Top + 1;

    // Remove scrollbar by setting buffer height to window height
    COORD newSize;
    newSize.X = bufferW;
    newSize.Y = windowH;
    SetConsoleScreenBufferSize(editor.hbuffer, newSize);

    editor.width = (int)(newSize.X);
    editor.height = (int)(newSize.Y);
}

// Asks user if they want to exit without saving. Writes file if answered yes.
void PromptFileNotSaved(Buffer *b)
{
    char prompt[512];
    strcpy(prompt, "Save file before closing? ");
    strcat(prompt, b->filepath);

    if (b->isFile && b->dirty)
        if (UiPromptYesNo(prompt, true) == UI_YES)
            EditorSaveFile();
}

// Returns pointer to file contents, NULL on fail. Size is written to.
char *EditorReadFile(const char *filepath, int *size)
{
    // Open file. EditorOpenFile does not create files and fails on file-not-found
    HANDLE file = CreateFileA(filepath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        Error("failed to load file");
        return NULL;
    }

    // Get file size and read file contents into string buffer
    DWORD bufSize = GetFileSize(file, NULL) + 1;
    DWORD read;
    char *buffer = MemAlloc(bufSize);
    if (!ReadFile(file, buffer, bufSize, &read, NULL))
    {
        Error("failed to read file");
        CloseHandle(file);
        return NULL;
    }

    CloseHandle(file);
    *size = bufSize - 1;
    buffer[bufSize - 1] = 0;
    return buffer;
}

// Prompts user for command input. If command is not NULL, it is set as the
// current command and cannot be removed by the user, used for shorthands.
void PromptCommand(char *command)
{
    // Todo: rewrite prompt command system

    SetStatus(NULL, NULL);
    char prompt[64] = ":";

    // Append initial command to text
    if (command != NULL)
    {
        strcat(prompt, command);
        strcat(prompt, " ");
    }

    UiResult res = UiGetTextInput(prompt, 64);
    char bufWithPrompt[res.length + 64];

    if (res.status != UI_OK)
        goto _return;

    // Split string by spaces
    strcpy(bufWithPrompt, prompt);
    strncat(bufWithPrompt, res.buffer, res.length);
    char *ptr = strtok(bufWithPrompt + 1, " ");
    char *args[16];
    int argc = 0;

    if (ptr == NULL)
        goto _return;

    while (ptr != NULL && argc < 16)
    {
        args[argc++] = ptr;
        ptr = strtok(NULL, " ");
    }

#define is_cmd(c) (!strcmp(c, args[0]))

    if (is_cmd("open"))
    {
        // Open file. Path is relative to executable
        if (argc == 1)
        {
            EditorOpenFile("");
        }
        else if (argc > 2)
            // Command error
            SetStatus(NULL, "too many args. usage: open [filepath]");
        else if (EditorOpenFile(args[1]) == RETURN_ERROR)
            // Try to open file with given name
            SetStatus(NULL, "file not found");
    }

    else if (is_cmd("help"))
        EditorShowHelp();
    else if (is_cmd("save"))
        EditorSaveFile();

    else if (is_cmd("theme") && argc > 1)
    {
        if (!LoadTheme(args[1], &colors))
            SetStatus(NULL, "theme not found");
    }

    else
        // Invalid command name
        SetStatus(NULL, "unknown command");

    Render();

_return:
    UiFreeResult(res);
}

void EditorSetMode(InputMode mode)
{
    if (mode == MODE_EDIT)
        CursorMove(curBuffer, -1, 0);

    editor.mode = mode;
}

extern char HELP_TEXT[];
void EditorShowHelp()
{
    Buffer *b = BufferLoadFile("Help", HELP_TEXT, strlen(HELP_TEXT));
    b->readOnly = true;
    EditorSetCurrentBuffer(b);
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

// Todo: (feature) file explorer
