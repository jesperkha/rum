// The editor is the main component of wim, including functionality for file IO, event handlers,
// syntax and highlight controls, configuration and more. The editor global instance is declared
// here and used by the entire core module.

#include "wim.h"

Editor editor = {0}; // Global instance used in core module

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

    editor.padH = 6; // Line numbers
    editor.padV = 2; // Status line

    editor.textW = editor.width - editor.padH;
    editor.textH = editor.height - editor.padV;
}

// Asks user if they want to exit without saving. Writes file if answered yes.
static void promptFileNotSaved()
{
    if (editor.info.fileOpen && editor.info.dirty)
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
    char *buffer = memAlloc(bufSize);
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
// Returns pointer to file data, NULL on error. Writes to size.
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

// Helper, creates line at row and writes content. Different from createLine as it
// knows the length of the line before hand and doesnt need to realloc.
static void writeLineToBuffer(int row, char *buffer, int length)
{
    // Realloc line array if out of bounds
    if (row >= editor.lineCap)
    {
        editor.lineCap += BUFFER_LINE_CAP;
        editor.lines = memRealloc(editor.lines, editor.lineCap * sizeof(Line));
        check_pointer(editor.lines, "bufferInsertLine");
    }

    Line line = {
        .row = row,
        .length = length - 1,
    };

    // Calculate cap size for the line length
    int l = DEFAULT_LINE_LENGTH;
    int cap = (length / l) * l + l;

    // Allocate chars and copy over line
    char *chars = memZeroAlloc(cap * sizeof(char));
    check_pointer(chars, "EditorOpenFile");
    strncpy(chars, buffer, length - 1);

    // Fill out line values and copy line to line array
    line.cap = cap;
    line.chars = chars;
    memcpy(&editor.lines[row], &line, sizeof(Line));

    // Increment number of line, position in buffer, and row
    editor.numLines = row + 1;
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
    CHECK("load editor themes", EditorLoadTheme("gruvbox"));
    CHECK("set title", SetConsoleTitleA(TITLE));

    // Editor size and scaling info
    updateSize();
    CONSOLE_SCREEN_BUFFER_INFO info;
    CHECK("get csb info", GetConsoleScreenBufferInfo(editor.hbuffer, &info));
    editor.initSize = (COORD){info.srWindow.Right, info.srWindow.Bottom};

    editor.scrollDx = 5;
    editor.scrollDy = 5;

    // Todo: load config from file
    editor.config = (Config){
        .matchParen = true,
        .syntaxEnabled = true,
        .useCRLF = true,
        .tabSize = 4,
    };

    // Initialize buffer
    editor.numLines = 0;
    editor.lineCap = BUFFER_LINE_CAP;
    editor.lines = memZeroAlloc(editor.lineCap * sizeof(Line));

    COORD maxSize = GetLargestConsoleWindowSize(editor.hbuffer);
    editor.renderBuffer = memAlloc(maxSize.X * maxSize.Y * 4);

    CHECK("alloc editor lines", editor.lines != NULL);
    CHECK("alloc render buffer", editor.renderBuffer != NULL);

    if (errors > 0)
        ExitProcess(EXIT_FAILURE);

    ScreenWrite("\033[?12l", 6); // Turn off cursor blinking
    EditorReset();               // Clear buffer and reset info

    UndoStackInit();

    // Handle command line options
    if (options.hasFile)
        EditorOpenFile(options.filename);
}

// Reset editor to empty file buffer. Resets editor Info struct.
void EditorReset()
{
    promptFileNotSaved();

    for (int i = 0; i < editor.numLines; i++)
        memFree(editor.lines[i].chars);

    editor.numLines = 0;
    editor.col = 0;
    editor.row = 0;
    editor.offx = 0;
    editor.offy = 0;
    editor.colMax = 0;

    BufferInsertLine(0);

    editor.info = (Info){
        .hasError = false,
        .fileOpen = false,
        .dirty = false,
        .syntaxReady = false,
    };

    SetStatus("[empty file]", NULL);
    Render();
}

void EditorExit()
{
    promptFileNotSaved();

    for (int i = 0; i < editor.numLines; i++)
        memFree(editor.lines[i].chars);

    memFree(editor.lines);
    memFree(editor.renderBuffer);
    UndoStackFree();
    SetConsoleScreenBufferSize(editor.hbuffer, editor.initSize);
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
            case 'u':
                Undo();
                break;

            case 'r':
                AppendEditAction(A_DELETE, editor.row, editor.col, "Hello");
                break;

            case 'q':
                EditorExit();

            case 'c':
                EditorPromptCommand(NULL);
                break;

            case 'o':
                EditorPromptCommand("open");
                break;

            case 'n':
                EditorReset();
                break;

            case 's':
                EditorSaveFile();
                break;

            case 'x':
                BufferDeleteLine(editor.row);
                CursorSetPos(0, editor.row, true);
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
            BufferScrollDown();
            break;

        case K_PAGEUP:
            BufferScrollUp();
            break;

        case K_BACKSPACE:
            BufferDeleteChar();
            break;

        case K_DELETE:
            TypingDeleteForward();
            break;

        case K_ENTER:
            BufferInsertLine(editor.row + 1);
            int length = editor.lines[editor.row + 1].length;
            BufferSplitLineDown(editor.row);
            CursorSetPos(length, editor.row + 1, false);
            if (editor.config.matchParen)
                TypingBreakParen();
            break;

        case K_TAB:
            TypingInsertTab();
            break;

        case K_ARROW_UP:
            CursorMove(0, -1);
            break;

        case K_ARROW_DOWN:
            CursorMove(0, 1);
            break;

        case K_ARROW_LEFT:
            CursorMove(-1, 0);
            break;

        case K_ARROW_RIGHT:
            CursorMove(1, 0);
            break;

        default:
            if (!(info.asciiChar < 32 || info.asciiChar > 126))
            {
                BufferWrite(&info.asciiChar, 1);
                if (editor.config.matchParen)
                    TypingMatchParen(info.asciiChar);
            }
        }

        Render();
    }

    return RETURN_SUCCESS;
}

// Loads file into buffer. Filepath must either be an absolute path
// or name of a file in the same directory as working directory.
Status EditorOpenFile(char *filepath)
{
    promptFileNotSaved();

    // Load syntax file for extension and set file type
    char *extension = strchr(filepath, '.');
    editor.info.fileType = FT_UNKNOWN;

    if (extension != NULL)
    {
#define FT(name, type)      \
    if (!strcmp(name, ext)) \
        editor.info.fileType = type;

        char *ext = extension + 1;
        editor.info.syntaxReady = EditorLoadSyntax(ext);
        FT("c", FT_C);
        FT("h", FT_C);
        FT("py", FT_PYTHON);
    }

    int size;
    char *buffer = readFile(filepath, &size);
    if (buffer == NULL)
        return RETURN_ERROR;

    char *newline;
    char *ptr = buffer;
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
    writeLineToBuffer(row, ptr, size - (ptr - buffer) + 1);
    memFree(buffer);

    editor.info.fileOpen = true;
    editor.info.dirty = false;
    editor.info.hasError = false;

    SetStatus(filepath, NULL);
    Render();
    return RETURN_SUCCESS;
}

// Writes content of buffer to filepath. Always truncates file.
Status EditorSaveFile()
{
    // Give file name before saving if blank
    if (!editor.info.fileOpen)
    {
        char buffer[64] = "Filename: ";
        memset(buffer + 10, 0, 54);
        if (UiTextInput(0, editor.height - 1, buffer, 64) != UI_OK)
            return RETURN_ERROR;

        if (strlen(buffer + 10) == 0)
            return RETURN_ERROR;

        SetStatus(buffer + 10, NULL);
        editor.info.fileOpen = true;
    }

    bool CRLF = editor.config.useCRLF;

    // Accumulate size of buffer by line length
    int size = 0;
    int newlineSize = CRLF ? 2 : 1;

    for (int i = 0; i < editor.numLines; i++)
        size += editor.lines[i].length + newlineSize;

    // Write to buffer, add newline for each line
    char buffer[size];
    char *ptr = buffer;
    for (int i = 0; i < editor.numLines; i++)
    {
        Line line = editor.lines[i];
        memcpy(ptr, line.chars, line.length);
        ptr += line.length;
        if (CRLF)
            *(ptr++) = '\r'; // CR
        *(ptr++) = '\n';     // LF
    }

    // Open file - truncate existing and write
    HANDLE file = CreateFileA(editor.info.filepath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        LogError("failed to open file");
        return RETURN_ERROR;
    }

    DWORD written; //            remove last newline
    if (!WriteFile(file, buffer, size - newlineSize, &written, NULL))
    {
        LogError("failed to write to file");
        CloseHandle(file);
        return RETURN_ERROR;
    }

    editor.info.dirty = false;
    CloseHandle(file);
    return RETURN_SUCCESS;
}

// Prompts user for command input. If command is not NULL, it is set as the
// current command and cannot be removed by the user, used for shorthands.
void EditorPromptCommand(char *command)
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
            // Create empty file
            EditorReset();
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
        if (!EditorLoadTheme(args[1]))
            SetStatus(NULL, "theme not found");
    }

    else
        // Invalid command name
        SetStatus(NULL, "unknown command");

    Render();
}

// Reads theme file and sets colorscheme if found.
Status EditorLoadTheme(const char *theme)
{
    int size;
    char *buffer = readConfigFile("runtime/themes.wim", &size);
    if (buffer == NULL)
        return RETURN_ERROR;

    char *ptr = buffer;
    while ((ptr - buffer) < size)
    {
        int nameLen = THEME_NAME_LEN;

        if (!strncmp(theme, ptr, nameLen))
        {
            memcpy(editor.colors, ptr + nameLen, COLORS_LENGTH);
            memFree(buffer);
            return RETURN_SUCCESS;
        }

        ptr += COLORS_LENGTH + nameLen;
    }

    memFree(buffer);
    return RETURN_ERROR;
}

// Loads syntax for given file extension, omitting the period.
// Writes to editor.syntaxTable struct, used by highlight function.
Status EditorLoadSyntax(const char *extension)
{
    int size;
    char *buffer = readConfigFile("runtime/syntax.wim", &size);
    if (buffer == NULL)
        return RETURN_ERROR;

    char *ptr = buffer;
    while (ptr != NULL && (ptr - buffer) < size)
    {
        int remainingLen = size - (ptr - buffer);

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

            memFree(buffer);
            return RETURN_SUCCESS;
        }

        ptr = memchr(ptr, '\n', remainingLen) + 1;
    }

    memFree(buffer);
    return RETURN_ERROR;
}