// The editor is the main component of wim, including functionality for file IO, event handlers,
// syntax and highlight controls, configuration and more. The editor global instance is declared
// here and used by the entire core module.

#include "wim.h"

Editor editor = {0}; // Global editor instance used in core module
Colors colors = {0}; // Global constant color palette loaded from theme.json
Config config = {0}; // Global constant config loaded from config.json

static void updateSize();
static void promptFileNotSaved();
static void promptCommand(char *command);

char *readFile(const char *filepath, int *size);
static char *readConfigFile(const char *file, int *size);

static bool loadSyntax(Buffer *b, char *filepath);

void error_exit(char *msg)
{
    printf("Error: %s\n", msg);
    LogError(msg);
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
    LogCreate();     // Enabled on debug only

    editor.hbuffer = INVALID_HANDLE_VALUE;

    // IMPORTANT:
    // order here matters a lot
    // buffer must be created before a file is loaded
    // csb must be set before any call to render/ScreenWrite etc
    // win32 does not give a shit if the handle is invalid and will
    // blame literally anything else (especially HeapFree for some reason)

    editor.buffers[0] = BufferNew();
    editor.activeBuffer = 0;
    editor.numBuffers = 1;

    if (!LoadTheme("gruvbox", &colors))
        error_exit("Failed to load default theme");

    if (!LoadConfig(&config))
        error_exit("Failed to load config file");

    if (options.hasFile)
    {
        if (!EditorOpenFile(options.filename))
            error_exit("File not found");
    }

    if ((editor.actions = list(EditorAction, UNDO_CAP)) == NULL)
        error_exit("failed to allocate undo stack");

    initTerm(); // Must be called before render
    Render();
    Log("Init");
}

void EditorFree()
{
    promptFileNotSaved();

    for (int i = 0; i < editor.numBuffers; i++)
        BufferFree(editor.buffers[i]);

    MemFree(editor.renderBuffer);
    ListFree(editor.actions);
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
        if (info.ctrlDown)
        {
            switch (info.asciiChar + 96) // Why this value?
            {
            case 'q':
                return RETURN_ERROR; // Exit

            case 'u':
                Undo();
                break;

            case 'r':
                Redo();
                break;

            case 'c':
                promptCommand(NULL);
                break;

            case 'o':
                promptCommand("open");
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

            Render();
            return RETURN_SUCCESS;
        }

    normal_input:
        switch (info.keyCode)
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
            TypingWriteChar(info.asciiChar);
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
        BufferFree(curBuffer);
        curBuffer = BufferNew();
        return RETURN_SUCCESS;
    }

    promptFileNotSaved();

    int size;
    char *buf = readFile(filepath, &size);
    if (buf == NULL)
        return RETURN_ERROR;

    // Change active buffer
    Buffer *newBuf = BufferLoadFile(filepath, buf, size);
    MemFree(curBuffer);
    curBuffer = newBuf;

    SetStatus(filepath, NULL);

    // Load syntax for file
    loadSyntax(newBuf, filepath);

    return RETURN_SUCCESS;
}

// Writes content of buffer to filepath. Always truncates file.
Status EditorSaveFile()
{
    if (!BufferSaveFile(curBuffer))
        return RETURN_ERROR;

    loadSyntax(curBuffer, curBuffer->filepath);
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
static void promptFileNotSaved()
{
    if (curBuffer->isFile && curBuffer->dirty)
        if (UiPromptYesNo("Save file before closing?", true) == UI_YES)
            EditorSaveFile();
}

// Returns pointer to file contents, NULL on fail. Size is written to.
char *readFile(const char *filepath, int *size)
{
    // Open file. EditorOpenFile does not create files and fails on file-not-found
    HANDLE file = CreateFileA(filepath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        LogError("failed to load file");
        return NULL;
    }

    // Get file size and read file contents into string buffer
    DWORD bufSize = GetFileSize(file, NULL) + 1;
    DWORD read;
    char *buffer = MemAlloc(bufSize);
    if (!ReadFile(file, buffer, bufSize, &read, NULL))
    {
        LogError("failed to read file");
        CloseHandle(file);
        return NULL;
    }

    CloseHandle(file);
    *size = bufSize - 1;
    buffer[bufSize - 1] = 0;
    return buffer;
}

// Looks for files in the directory of the executable, eg. config, runtime etc.
// Returns pointer to file data, NULL on error. Writes to size. Remember to free!
static char *readConfigFile(const char *file, int *size)
{
    const int pathSize = 512;

    // Concat path to executable with filepath
    char path[pathSize];
    int len = GetModuleFileNameA(NULL, path, pathSize);
    for (int i = len; i > 0 && path[i] != '\\'; i--)
        path[i] = 0;

    strcat(path, file);
    return readFile(path, size);
}

// Prompts user for command input. If command is not NULL, it is set as the
// current command and cannot be removed by the user, used for shorthands.
static void promptCommand(char *command)
{
    SetStatus(NULL, NULL);
    char text[64] = ":";

    // Append initial command to text
    if (command != NULL)
    {
        strcat(text, command);
        strcat(text, " ");
    }

    int status = UiTextInput(0, editor.height - 1, text, 64);
    if (status != UI_OK)
        return;

    // Split string by spaces
    char *ptr = strtok(text + 1, " ");
    char *args[16];
    int argc = 0;

    if (ptr == NULL)
        return;

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
}

// Loads syntax for given filepath. Sets the buffers syntax table if successful.
// If not successful, buffer.syntaxReady is false.
static bool loadSyntax(Buffer *b, char *filepath)
{
    if (b->syntaxReady)
        MemFree(b->syntaxTable);

    SyntaxTable *table = MemZeroAlloc(sizeof(SyntaxTable));
    b->syntaxReady = false;

    char extension[8];
    StrFileExtension(extension, filepath);

    int size;
    char *buf = readConfigFile("runtime/syntax.wim", &size);
    if (buf == NULL || size == 0)
        return false;

    char *ptr = buf;
    while (ptr != NULL && (ptr - buf) < size)
    {
        int remainingLen = size - (ptr - buf);

        if (!strncmp(extension, ptr, SYNTAX_NAME_LEN))
        {
            // Copy extension name
            strcpy(table->extension, ptr);

            // Copy keyword and type segment
            for (int j = 0; j < 2; j++)
            {
                char *start = ptr;
                ptr = memchr(ptr, '?', remainingLen) + 1;

                int length = ptr - start;
                memcpy(table->words[j], start, length);
                table->numWords[j] = length;
            }

            MemFree(buf);
            b->syntaxReady = true;
            b->syntaxTable = table;
            return true;
        }

        ptr = memchr(ptr, '\n', remainingLen) + 1;
    }

    LogError("file extension not supported");
    MemFree(buf);
    return false;
}
