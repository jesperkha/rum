// The editor is the main component of wim, including functionality for file IO, event handlers,
// syntax and highlight controls, configuration and more. The editor global instance is declared
// here and used by the entire core module.

#include "wim.h"

Editor editor = {0}; // Global editor instance used in core module
Colors colors = {0}; // Global color palette loaded from theme.json
Config config = {0};

Buffer buffer = {0}; // Debug

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
    if (buffer.isFile && buffer.dirty)
        if (UiPromptYesNo("Save file before closing?", true) == UI_YES)
            EditorSaveFile();
}

// Returns pointer to file contents, NULL on fail. Size is written to.
static char *readFile(const char *filepath, int *size)
{
    // Open file. EditorOpenFile does not create files and fails on file-not-found
    HANDLE file = CreateFileA(filepath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        LogError("failed to load file");
        return NULL;
    }

    // Get file size and read file contents into string buffer
    DWORD bufSize = GetFileSize(file, NULL);
    DWORD read;
    char *buffer = MemAlloc(bufSize);
    if (!ReadFile(file, buffer, bufSize, &read, NULL))
    {
        LogError("failed to read file");
        CloseHandle(file);
        return NULL;
    }

    CloseHandle(file);
    *size = bufSize;
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

// Loads config file into editor config object.
static Status editorLoadConfig()
{
    int size;
    char *buffer = readConfigFile("runtime/config.wim", &size);
    if (buffer == NULL || size == 0)
        return RETURN_ERROR;

    memcpy(&config, buffer, min(sizeof(Config), size));
    MemFree(buffer);
    return RETURN_SUCCESS;
}

// Helper, creates line at row and writes content. Different from createLine as it
// knows the length of the line before hand and doesnt need to realloc.
static void writeLineToBuffer(int row, char *buf, int length)
{
    // Realloc line array if out of bounds
    if (row >= buffer.lineCap)
    {
        buffer.lineCap += BUFFER_DEFAULT_LINE_CAP;
        buffer.lines = MemRealloc(buffer.lines, buffer.lineCap * sizeof(Line));
        check_pointer(buffer.lines, "bufferInsertLine");
    }

    Line line = {
        .row = row,
        .length = length - 1,
    };

    // Calculate cap size for the line length
    int l = LINE_DEFAULT_LENGTH;
    int cap = (length / l) * l + l;

    // Allocate chars and copy over line
    char *chars = MemZeroAlloc(cap * sizeof(char));
    check_pointer(chars, "EditorOpenFile");
    strncpy(chars, buf, length - 1);

    // Fill out line values and copy line to line array
    line.cap = cap;
    line.chars = chars;
    memcpy(&buffer.lines[row], &line, sizeof(Line));

    // Increment number of line, position in buffer, and row
    buffer.numLines = row + 1;
}

// Prompts user for command input. If command is not NULL, it is set as the
// current command and cannot be removed by the user, used for shorthands.
static void editorPromptCommand(char *command)
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

    if (is_cmd("exit") && argc == 1) // Exit
        // Exit editor
        EditorExit();

    else if (is_cmd("open"))
    {
        // Open file. Path is relative to executable
        if (argc == 1)
            // Todo: Create empty file
            ;
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
        if (!editorLoadTheme(args[1]))
            SetStatus(NULL, "theme not found");
    }

    else
        // Invalid command name
        SetStatus(NULL, "unknown command");

    Render();
}

// Reads theme file and sets colorscheme if found.
static Status editorLoadTheme(char *theme)
{
    int size;
    char *buffer = readConfigFile("runtime/themes.wim", &size);
    if (buffer == NULL || size == 0)
        return RETURN_ERROR;

    char *ptr = StrMemStr(buffer, theme, size);
    if (ptr == NULL)
    {
        MemFree(buffer);
        return RETURN_ERROR;
    }

    memcpy(&colors, ptr, sizeof(Colors));
    MemFree(buffer);
    return RETURN_SUCCESS;
}

// Loads syntax for given file extension, omitting the period.
// Writes to editor.syntaxTable struct, used by highlight function.
static Status editorLoadSyntax(char *extension)
{
    int size;
    char *buf = readConfigFile("runtime/syntax.wim", &size);
    if (buf == NULL || size == 0)
        return RETURN_ERROR;

    char *ptr = buf;
    while (ptr != NULL && (ptr - buf) < size)
    {
        int remainingLen = size - (ptr - buf);

        if (!strncmp(extension, ptr, SYNTAX_NAME_LEN))
        {
            // Copy extension name
            strcpy(editor.syntaxTable.ext, ptr);

            // Copy keyword and type segment
            for (int j = 0; j < 2; j++)
            {
                char *start = ptr;
                ptr = memchr(ptr, '?', remainingLen) + 1;

                int length = ptr - start;
                memcpy(editor.syntaxTable.syn[j], start, length);
                editor.syntaxTable.len[j] = length;
            }

            MemFree(buf);

            // Load syntax file for extension and set file type
            // editor.info.fileType = FT_UNKNOWN;

#define FT(name, type)            \
    if (!strcmp(name, extension)) \
    // editor.info.fileType = type;

            FT("c", FT_C);
            FT("h", FT_C);
            FT("py", FT_PYTHON);

            // editor.info.syntaxReady = editor.info.fileType != FT_UNKNOWN;
            return RETURN_SUCCESS;
        }

        ptr = memchr(ptr, '\n', remainingLen) + 1;
    }

    MemFree(buf);
    buffer.syntaxReady = false;
    return RETURN_ERROR;
}

// Populates editor global struct and creates empty file buffer. Exits on error.
void EditorInit(CmdOptions options)
{
    system("color");

#ifdef DEBUG
    // Debug: clear log file
    FILE *f = fopen("log", "w");
    fclose(f);
#endif

    int errors = 0;

#define CHECK(what, v)                                  \
    if (!(v))                                           \
    {                                                   \
        fprintf(stderr, "error: failed to %s\n", what); \
        errors++;                                       \
    }

    editor.hstdin = GetStdHandle(STD_INPUT_HANDLE);
    editor.hbuffer = CreateConsoleScreenBuffer(GENERIC_WRITE | GENERIC_READ, 0, NULL, 1, NULL);

    // Checks for basic I/O initialization, should never fail
    CHECK("get csb handle", editor.hbuffer != INVALID_HANDLE_VALUE);
    CHECK("get stdin handle", editor.hstdin != INVALID_HANDLE_VALUE);
    CHECK("set active buffer", SetConsoleActiveScreenBuffer(editor.hbuffer));
    CHECK("set raw input mode", SetConsoleMode(editor.hstdin, 0));
    CHECK("flush input buffer", FlushConsoleInputBuffer(editor.hstdin));

    // Other
    CHECK("load editor themes", editorLoadTheme("gruvbox"));
    CHECK("set title", SetConsoleTitleA(TITLE));

    // Editor size and scaling info
    updateSize();
    CONSOLE_SCREEN_BUFFER_INFO info;
    CHECK("get csb info", GetConsoleScreenBufferInfo(editor.hbuffer, &info));
    // editor.initSize = (COORD){info.srWindow.Right, info.srWindow.Bottom};

    editorLoadConfig();

    // Debug
    buffer = *BufferNew();

    COORD maxSize = GetLargestConsoleWindowSize(editor.hbuffer);
    editor.renderBuffer = MemAlloc(maxSize.X * maxSize.Y * 4);

    editor.actions = list(EditorAction, UNDO_CAP);

    CHECK("alloc editor lines", buffer.lines != NULL);
    CHECK("alloc render buffer", editor.renderBuffer != NULL);
    CHECK("alloc action stack", editor.actions != NULL);

    if (errors > 0)
        ExitProcess(EXIT_FAILURE);

    ScreenWrite("\033[?12l", 6); // Turn off cursor blinking
    // EditorReset();               // Clear buffer and reset info

    // Handle command line options
    if (options.hasFile)
        EditorOpenFile(options.filename);

    SetStatus("[empty file]", NULL);
    Render();
}

void EditorExit()
{
    promptFileNotSaved();

    for (int i = 0; i < buffer.numLines; i++)
        MemFree(buffer.lines[i].chars);

    MemFree(buffer.lines);
    MemFree(editor.renderBuffer);
    ListFree(editor.actions);
    // SetConsoleScreenBufferSize(editor.hbuffer, editor.initSize);
    CloseHandle(editor.hbuffer);
    ExitProcess(EXIT_SUCCESS);
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
                EditorExit();

            case 'u':
                Undo();
                break;

            case 'r':
                Redo();
                break;

            case 'c':
                editorPromptCommand(NULL);
                break;

            case 'o':
                editorPromptCommand("open");
                break;

            case 'n':
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
            EditorExit();

        case K_PAGEDOWN:
            BufferInsertLine(&buffer, buffer.cursor.row, "Hello :)");
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
            CursorMove(&buffer, 0, -1);
            break;

        case K_ARROW_DOWN:
            CursorMove(&buffer, 0, 1);
            break;

        case K_ARROW_LEFT:
            CursorMove(&buffer, -1, 0);
            break;

        case K_ARROW_RIGHT:
            CursorMove(&buffer, 1, 0);
            break;

        default:
            TypingWriteChar(info.asciiChar);
        }

        Render();
        return RETURN_SUCCESS;
    }

    return RETURN_SUCCESS; // Unknown event
}

// Loads file into buffer. Filepath must either be an absolute path
// or name of a file in the same directory as working directory.
Status EditorOpenFile(char *filepath)
{
    promptFileNotSaved();

    int size;
    char *buf = readFile(filepath, &size);
    if (buf == NULL)
        return RETURN_ERROR;

    char *newline;
    char *ptr = buf;
    int row = 0;
    while ((newline = strstr(ptr, "\n")) != NULL)
    {
        // Get distance from current pos in buffer and found newline
        // Then strncpy the line into the line char buffer
        int length = newline - ptr;
        writeLineToBuffer(row, ptr, length);
        ptr += length + 1;
        row++;
    }

    // Write last line of file
    writeLineToBuffer(row, ptr, size - (ptr - buf) + 1);
    MemFree(buf);

    buffer.isFile = true;
    buffer.dirty = false;

    SetStatus(filepath, NULL);
    Render();
    return RETURN_SUCCESS;
}

// Writes content of buffer to filepath. Always truncates file.
Status EditorSaveFile()
{
    // Give file name before saving if blank
    if (!buffer.isFile)
    {
        char buf[64] = "Filename: ";
        memset(buf + 10, 0, 54);
        if (UiTextInput(0, editor.height - 1, buf, 64) != UI_OK)
            return RETURN_ERROR;

        if (strlen(buf + 10) == 0)
            return RETURN_ERROR;

        SetStatus(buf + 10, NULL);
        buffer.isFile = true;
    }

    bool CRLF = config.useCRLF;

    // Accumulate size of buffer by line length
    int size = 0;
    int newlineSize = CRLF ? 2 : 1;

    for (int i = 0; i < buffer.numLines; i++)
        size += buffer.lines[i].length + newlineSize;

    // Write to buffer, add newline for each line
    char buf[size];
    char *ptr = buf;
    for (int i = 0; i < buffer.numLines; i++)
    {
        Line line = buffer.lines[i];
        memcpy(ptr, line.chars, line.length);
        ptr += line.length;
        if (CRLF)
            *(ptr++) = '\r'; // CR
        *(ptr++) = '\n';     // LF
    }

    // Open file - truncate existing and write
    HANDLE file = CreateFileA(buffer.filepath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        LogError("failed to open file");
        return RETURN_ERROR;
    }

    DWORD written; //            remove last newline
    if (!WriteFile(file, buf, size - newlineSize, &written, NULL))
    {
        LogError("failed to write to file");
        CloseHandle(file);
        return RETURN_ERROR;
    }

    buffer.dirty = false;
    CloseHandle(file);
    return RETURN_SUCCESS;
}
