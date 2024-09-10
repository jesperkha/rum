// The editor is the main component of rum, including functionality for file IO, event handlers,
// syntax and highlight controls, configuration and more. The editor global instance is declared
// here and used by the entire core module.

#include "rum.h"

Editor editor; // Global editor instance used in core module
Colors colors; // Global constant color palette loaded from theme.json
Config config; // Global constant config loaded from config.json

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
        ErrorExit("Failed to load default theme");

    if (!LoadConfig(&config))
        ErrorExit("Failed to load config file");

    if (options.hasFile)
    {
        if (!EditorOpenFile(options.filename))
            ErrorExitf("File '%s' not found", options.filename);
    }

    config.rawMode = options.rawMode;

    memset(editor.padBuffer, ' ', MAX_PADDING);

    TermInit(&editor); // Must be called before render
    SetStatus("[empty file]", NULL);
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

#ifdef OS_WINDOWS
    CloseHandle(editor.hbuffer);
#endif

    Log("Editor free successful");
}

// Waits for input and takes action for insert mode.
Status EditorHandleInput()
{
    InputInfo info;
    if (ReadTerminalInput(&info) == RETURN_ERROR)
        return RETURN_ERROR;

    if (info.eventType == INPUT_WINDOW_RESIZE)
    {
        TermUpdateSize(&editor);
        Render();
        return RETURN_SUCCESS;
    }

    if (info.ctrlDown && HandleCtrlInputs(&info))
    {
        Render();
        return RETURN_SUCCESS;
    }

    if (info.eventType == INPUT_KEYDOWN)
    {
        Status s;
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

        default:
            Panicf("Unhandled input mode %d", editor.mode);
            break;
        }

        Render();
        return s;
    }

    return RETURN_SUCCESS; // Unhandled event
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
    char *buf = OsReadFile(filepath, &size);
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
    if (editor.mode == MODE_INSERT && mode == MODE_EDIT)
        CursorMove(curBuffer, -1, 0);

    if (editor.mode == MODE_VISUAL)
        CursorSetPos(curBuffer, curBuffer->hlA.col, curBuffer->hlA.row, false);

    // Toggle highlighting (purely visual)
    curBuffer->highlight = mode == MODE_VISUAL;
    curBuffer->cursor.visible = mode != MODE_VISUAL;

    if (mode == MODE_VISUAL)
    {
        curBuffer->hlA = (CursorPos){curRow, curCol};
        curBuffer->hlB = (CursorPos){curRow, curCol};
    }

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
