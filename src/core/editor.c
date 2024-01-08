#include "wim.h"

Editor editor;

Editor *editorGetHandle()
{
    return &editor;
}

static int errors = 0;

void Error(const char *msg)
{
    fprintf(stderr, "error: %s\n", msg);
    errors++;
}

// Populates editor global struct and creates empty file buffer. Exits on error.
void editorInit()
{
    system("color");
    
    // Debug: clear log file
    FILE *f = fopen("log", "w");
    fclose(f);

    editor.hstdin = GetStdHandle(STD_INPUT_HANDLE);
    editor.hbuffer = CreateConsoleScreenBuffer(GENERIC_WRITE | GENERIC_READ, 0, NULL, 1, NULL);

#define CHECK(what, v) if (!(v)) Error("failed to "what)

    CHECK("get csb handle",     editor.hbuffer != INVALID_HANDLE_VALUE);
    CHECK("get stdin handle",   editor.hstdin != INVALID_HANDLE_VALUE);
    CHECK("load editor themes", editorLoadTheme("gruvbox"));
    CHECK("set active buffer",  SetConsoleActiveScreenBuffer(editor.hbuffer));
    CHECK("set raw input mode", SetConsoleMode(editor.hstdin, 0));
    CHECK("flush input buffer", FlushConsoleInputBuffer(editor.hstdin));
    CHECK("set title",          SetConsoleTitleA(TITLE));

    editorUpdateSize();
    CONSOLE_SCREEN_BUFFER_INFO info;
    CHECK("get csb info", GetConsoleScreenBufferInfo(editor.hbuffer, &info));

    editor.initSize = (COORD){info.srWindow.Right, info.srWindow.Bottom};
    editor.scrollDx = 5;
    editor.scrollDy = 5;

    editor.config = (Config){
        .matchParen = true,
        .syntaxEnabled = true,
        .useCRLF = true,
        .tabSize = 4,
    };

    editor.numLines = 0;
    editor.lineCap = BUFFER_LINE_CAP;
    editor.lines = memZeroAlloc(editor.lineCap * sizeof(Line));

    COORD maxSize = GetLargestConsoleWindowSize(editor.hbuffer);
    editor.renderBuffer = memAlloc(maxSize.X * maxSize.Y * 4);

    CHECK("alloc editor lines",  editor.lines != NULL);
    CHECK("alloc render buffer", editor.renderBuffer != NULL);

    if (errors > 0)
        ExitProcess(EXIT_FAILURE);

    editorReset();
    screenBufferWrite("\033[?12l", 6); // Turn off cursor blinking
    renderBuffer();
}

// Reset editor to empty file buffer. Resets editor Info struct.
void editorReset()
{
    editorPromptFileNotSaved();

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

    statusBarUpdate("[empty file]", NULL);
}

void editorExit()
{
    editorPromptFileNotSaved();

    for (int i = 0; i < editor.numLines; i++)
        memFree(editor.lines[i].chars);

    memFree(editor.lines);
    memFree(editor.renderBuffer);
    SetConsoleScreenBufferSize(editor.hbuffer, editor.initSize);
    CloseHandle(editor.hbuffer);
    ExitProcess(EXIT_SUCCESS);
}

// Update editor and screen buffer size.
void editorUpdateSize()
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

void editorWriteAt(int x, int y, const char *text)
{
    cursorHide();
    cursorTempPos(x, y);
    screenBufferWrite(text, strlen(text));
    cursorRestore();
    cursorShow();
}

// Hangs when waiting for input.
int editorReadInput(InputInfo *info)
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

int editorHandleInput()
{
    InputInfo info;
    if (editorReadInput(&info) == RETURN_ERROR)
        return RETURN_ERROR;

    if (info.eventType == INPUT_WINDOW_RESIZE)
    {
        editorUpdateSize();
        renderBuffer();
        return RETURN_SUCCESS;
    }

    if (info.eventType == INPUT_KEYDOWN)
    {
        if (info.ctrlDown)
        {
            switch (info.asciiChar + 96) // Why this value?
            {
            case 'q':
                editorExit();

            case 'c':
                editorCommand(NULL);
                break;
            
            case 'o':
                editorCommand("open");
                break;
            
            case 'n':
                editorReset();
                break;
            
            case 's':
                editorSaveFile();
                break;
            
            case 'x':
                BufferDeleteLine(editor.row);
                cursorSetPos(0, editor.row, true);
                break;
            
            default:
                goto normal_input;
            }

            renderBuffer();
            return RETURN_SUCCESS;
        }

    normal_input:

        switch (info.keyCode)
        {
        case K_ESCAPE:
            editorExit();

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
            typingDeleteForward();
            break;

        case K_ENTER:
            BufferInsertLine(editor.row + 1);
            int length = editor.lines[editor.row + 1].length;
            BufferSplitLineDown(editor.row);
            cursorSetPos(length, editor.row + 1, false);
            if (editor.config.matchParen)
                typingBreakParen();
            break;

        case K_TAB:
            typingInsertTab();
            break;

        case K_ARROW_UP:
            cursorMove(0, -1);
            break;

        case K_ARROW_DOWN:
            cursorMove(0, 1);
            break;

        case K_ARROW_LEFT:
            cursorMove(-1, 0);
            break;

        case K_ARROW_RIGHT:
            cursorMove(1, 0);
            break;

        default:
            BufferWrite(&info.asciiChar, 1);
            if (editor.config.matchParen)
                typingMatchParen(info.asciiChar);
        }

        renderBuffer();
    }

    return RETURN_SUCCESS;
}

// Asks user if they want to exit without saving. Writes file if answered yes.
void editorPromptFileNotSaved()
{
    if (editor.info.fileOpen && editor.info.dirty)
        if (uiPromptYesNo("Save file before closing?", true) == UI_YES)
            editorSaveFile();
}

// Returns pointer to file contents, NULL on fail. Size is written to.
static char *readFile(const char *filepath, int *size)
{
    // Open file. editorOpenFile does not create files and fails on file-not-found
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
        .idx = 0,
    };

    // Calculate cap size for the line length
    int l = DEFAULT_LINE_LENGTH;
    int cap = (length / l) * l + l;

    // Allocate chars and copy over line
    char *chars = memZeroAlloc(cap * sizeof(char));
    check_pointer(chars, "editorOpenFile");
    strncpy(chars, buffer, length - 1);

    // Fill out line values and copy line to line array
    line.cap = cap;
    line.chars = chars;
    memcpy(&editor.lines[row], &line, sizeof(Line));

    // Increment number of line, position in buffer, and row
    editor.numLines = row + 1;
}

// Loads file into buffer. Filepath must either be an absolute path
// or name of a file in the same directory as working directory.
int editorOpenFile(char *filepath)
{
    editorPromptFileNotSaved();

    // Load syntax file for extension and set file type
    char *extension = strchr(filepath, '.');
    editor.info.fileType = FT_UNKNOWN;

    if (extension != NULL)
    {
    #define FT(name, type) if (!strcmp(name, ext)) editor.info.fileType = type;
        char *ext = extension+1;
        editor.info.syntaxReady = editorLoadSyntax(ext);
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

    renderBuffer();
    statusBarUpdate(filepath, NULL);
    return RETURN_SUCCESS;
}

// Writes content of buffer to filepath. Always truncates file.
int editorSaveFile()
{
    // Give file name before saving if blank
    if (!editor.info.fileOpen)
    {
        char buffer[64] = "Filename: ";
        memset(buffer+10, 0, 54);
        if (uiTextInput(0, editor.height-1, buffer, 64) != UI_OK)
            return RETURN_ERROR;
        
        if (strlen(buffer+10) == 0)
            return RETURN_ERROR;
            
        statusBarUpdate(buffer+10, NULL);
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
    if (!WriteFile(file, buffer, size-newlineSize, &written, NULL))
    {
        LogError("failed to write to file");
        CloseHandle(file);
        return RETURN_ERROR;
    }

    editor.info.dirty = false;
    CloseHandle(file);
    return RETURN_SUCCESS;
}

// Waits for user text input and runs command
void editorCommand(char *command)
{
    statusBarClear();
    char text[64] = ":";

    // Append initial command to text
    if (command != NULL)
    {
        strcat(text, command);
        strcat(text, " ");
    }

    int status = uiTextInput(0, editor.height - 1, text, 64);
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
        editorExit();

    else if (is_cmd("open"))
    {
        // Open file. Path is relative to executable
        if (argc == 1)
            // Create empty file
            editorReset();
        else if (argc > 2)
            // Command error
            statusBarUpdate(NULL, "too many args. usage: open [filepath]");
        else if (editorOpenFile(args[1]) == RETURN_ERROR)
            // Try to open file with given name
            statusBarUpdate(NULL, "file not found");
    }

    else if (is_cmd("save"))
        editorSaveFile();
    
    else if (is_cmd("theme") && argc > 1)
    {
        if (!editorLoadTheme(args[1]))
            statusBarUpdate(NULL, "theme not found");
    }

    else
        // Invalid command name
        statusBarUpdate(NULL, "unknown command");
}

// Helper, returns char pointer to file contents, NULL on error. Writes to size.
// The file must be located within the runtime directory.
static char *readConfigFile(const char *file, int *size)
{
    // Concat path to executable with filepath
    char path[MAX_PATH + 32];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    for (int i = MAX_PATH-1; i > 0 && path[i] != '\\'; i--)
        path[i] = 0;

    strcat(path, "runtime/");
    strcat(path, file);
    return readFile(path, size);
}

// Reads theme file and sets colorscheme if found.
int editorLoadTheme(const char *theme)
{
    int size;
    char *buffer = readConfigFile("themes.wim", &size);
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
int editorLoadSyntax(const char *extension)
{
    int size;
    char *buffer = readConfigFile("syntax.wim", &size);
    if (buffer == NULL)
        return RETURN_ERROR;

    char *ptr = buffer;
    while (ptr != NULL && (ptr - buffer) < size)
    {
        int remainingLen = size - (ptr-buffer);

        if (!strncmp(extension, ptr, SYNTAX_NAME_LEN))
        {
            // Copy extension name
            strcpy(editor.syntaxTable.ext, ptr);

            // Copy keyword and type segment
            for (int j = 0; j < 2; j++)
            {
                char *start = ptr;
                ptr = memchr(ptr, '?', remainingLen)+1;

                int length = ptr - start;
                memcpy(editor.syntaxTable.syn[j], start, length);
                editor.syntaxTable.len[j] = length;
            }

            memFree(buffer);
            return RETURN_SUCCESS;
        }

        ptr = memchr(ptr, '\n', remainingLen)+1;
    }

    memFree(buffer);
    return RETURN_ERROR;
}

// ---------------------- SCREEN BUFFER ----------------------

// Writes at cursor position
void screenBufferWrite(const char *string, int length)
{
    DWORD written;
    if (!WriteConsoleA(editor.hbuffer, string, length, &written, NULL) || written != length)
    {
        LogError("Failed to write to screen buffer");
        editorExit();
    }
}

void screenBufferBg(int col)
{
    screenBufferWrite("\x1b[48;2;", 7);
    screenBufferWrite(editor.colors+col, 11);
    screenBufferWrite("m", 1);
}

void screenBufferFg(int col)
{
    screenBufferWrite("\x1b[38;2;", 7);
    screenBufferWrite(editor.colors+col, 11);
    screenBufferWrite("m", 1);
}

void screenBufferClearLine(int row)
{
    COORD pos = {0, row};
    DWORD written;
    FillConsoleOutputCharacterA(editor.hbuffer, ' ', editor.width, pos, &written);
}

void screenBufferClearAll()
{
    DWORD written;
    COORD pos = {0, 0};
    int size = editor.width * editor.height;
    FillConsoleOutputCharacterA(editor.hbuffer, ' ', size, pos, &written);
}

// ---------------------- CURSOR ----------------------

void cursorShow()
{
    CONSOLE_CURSOR_INFO info = {100, true};
    SetConsoleCursorInfo(editor.hbuffer, &info);
}

void cursorHide()
{
    CONSOLE_CURSOR_INFO info = {100, false};
    SetConsoleCursorInfo(editor.hbuffer, &info);
}

// Adds x,y to position
void cursorMove(int x, int y)
{
    cursorSetPos(editor.col + x, editor.row + y, true);
}

// KeepX is true when the cursor should keep the current max width
// when moving vertically, only really used with cursorMove.
void cursorSetPos(int x, int y, bool keepX)
{
    int dx = x - editor.col;
    int dy = y - editor.row;
    BufferScroll(dy); // Scroll by cursor offset

    editor.col = x;
    editor.row = y;

    Line line = editor.lines[editor.row];

    // Keep cursor within bounds
    if (editor.col < 0)
        editor.col = 0;
    if (editor.col > line.length)
        editor.col = line.length;
    if (editor.row < 0)
        editor.row = 0;
    if (editor.row > editor.numLines - 1)
        editor.row = editor.numLines - 1;
    if (editor.row - editor.offy > editor.textH)
        editor.row = editor.offy + editor.textH - editor.scrollDy;

    // Get indent for current line
    int i = 0;
    editor.indent = 0;
    while (i < editor.col && line.chars[i++] == ' ')
        editor.indent = i;

    // Keep cursor x when moving vertically
    if (keepX)
    {
        if (dy != 0)
        {
            if (editor.col > editor.colMax)
                editor.colMax = editor.col;
            if (editor.colMax <= line.length)
                editor.col = editor.colMax;
            if (editor.colMax > line.length)
                editor.col = line.length;
        }
        if (dx != 0)
            editor.colMax = editor.col;
    }
}

// Sets the cursor pos without additional stuff happening.
// The editor position is not updated so cursor returns to
// previous position when render is called.
void cursorTempPos(int x, int y)
{
    COORD pos = {x, y};
    SetConsoleCursorPosition(editor.hbuffer, pos);
}

// Restores cursor position to editor pos.
void cursorRestore()
{
    cursorSetPos(editor.col, editor.row, false);
}



// ---------------------- TYPING HELPERS ----------------------

const char begins[] = "\"'({[";
const char ends[] = "\"')}]";

void typingInsertTab()
{
    for (int i = 0; i < editor.config.tabSize; i++)
        BufferWrite(" ", 1);
}

// Matches braces, parens, strings etc with written char
void typingMatchParen(char c)
{
    Line line = editor.lines[editor.row];

    for (int i = 0; i < strlen(begins); i++)
    {
        if (c == ends[i] && line.chars[editor.col] == ends[i])
        {
            typingDeleteForward();
            break;
        }
        
        if (c == begins[i])
        {
            BufferWrite((char*)&ends[i], 1);
            cursorMove(-1, 0);
            break;
        }
    }
}

// When pressing enter after a paren, indent and move mathing paren to line below.
void typingBreakParen()
{
    Line line1 = editor.lines[editor.row];
    Line line2 = editor.lines[editor.row - 1];

    for (int i = 2; i < strlen(begins); i++)
    {
        char a = begins[i];
        char b = ends[i];

        if (line2.chars[line2.length-1] == a)
        {
            typingInsertTab();

            if (line1.chars[editor.col] == b)
            {
                BufferInsertLine(editor.row + 1);
                BufferSplitLineDown(editor.row);
            }

            return;
        }
    }
}

// Same as delete key on keyboard.
void typingDeleteForward()
{
    if (editor.col == editor.lines[editor.row].length)
    {
        if (editor.row == editor.numLines - 1)
            return;

        cursorHide();
        cursorSetPos(0, editor.row + 1, false);
    }
    else
    {
        cursorHide();
        cursorMove(1, 0);
    }

    BufferDeleteChar();
    cursorShow();
}

// ---------------------- RENDER ----------------------

void charbufClear(CharBuffer *buf)
{
    buf->pos = buf->buffer;
    buf->lineLength = 0;
}

void charbufAppend(CharBuffer *buf, char *src, int length)
{
    memcpy(buf->pos, src, length);
    buf->pos += length;
    buf->lineLength += length;
}

void charbufNextLine(CharBuffer *buf)
{
    int size = editor.width - buf->lineLength;
    for (int i = 0; i < size; i++)
        *(buf->pos++) = ' ';
    buf->lineLength = 0;
}

void charbufColor(CharBuffer *buf, char *col)
{
    int length = strlen(col);
    memcpy(buf->pos, col, length);
    buf->pos += length;
}

void charbufBg(CharBuffer *buf, int col)
{
    charbufColor(buf, "\x1b[48;2;");
    charbufColor(buf, editor.colors + col);
    charbufColor(buf, "m");
}

void charbufFg(CharBuffer *buf, int col)
{
    charbufColor(buf, "\x1b[38;2;");
    charbufColor(buf, editor.colors + col);
    charbufColor(buf, "m");
}

void charbufRender(CharBuffer *buf, int x, int y)
{
    cursorHide();
    cursorTempPos(x, y);
    screenBufferWrite(buf->buffer, buf->pos - buf->buffer);
    cursorRestore();
    cursorShow();
}

#define color(col) charbufColor(&buf, col);
#define bg(col) charbufBg(&buf, col);
#define fg(col) charbufFg(&buf, col);

char padding[256] = {[0 ... 255] = ' '};

void renderBuffer()
{
    CharBuffer buf = {
        .buffer = editor.renderBuffer,
        .pos = editor.renderBuffer,
        .lineLength = 0,
    };

    // Draw lines
    for (int i = 0; i < editor.textH; i++)
    {
        int row = i + editor.offy;
        if (row >= editor.numLines)
            break;

        bg(COL_BG0);
        fg(COL_BG2);

        if (editor.row == row)
        {
            bg(COL_BG1);
            fg(COL_YELLOW);
        }

        // Line number
        char numbuf[12];
        // Assert short to avoid compiler error
        sprintf(numbuf, " %4d ", (short)(row + 1));
        charbufAppend(&buf, numbuf, 6);

        fg(COL_FG0);

        // Line contents
        editor.offx = max(editor.col - editor.textW + editor.scrollDx, 0);
        int lineLength = editor.lines[row].length - editor.offx;
        int renderLength = min(lineLength, editor.textW);
        char *lineBegin = editor.lines[row].chars + editor.offx;

        if (lineLength <= 0)
        {
            charbufNextLine(&buf);
            color(COL_RESET);
            continue;
        }

        if (editor.config.syntaxEnabled && editor.info.syntaxReady)
        {
            // Generate syntax highlighting for line and get new byte length
            int newLength;
            char *line = highlightLine(lineBegin, renderLength, &newLength);
            charbufAppend(&buf, line, newLength);

            // Subtract added highlight strings from line length as they are 0-width
            int diff = newLength - lineLength;
            buf.lineLength -= diff;
        }
        else
            charbufAppend(&buf, lineBegin, renderLength);

        // Add padding at end for horizontal scroll
        int off = editor.textW - lineLength;
        if (editor.offx > 0 && off > 0)
            charbufAppend(&buf, padding, off);

        charbufNextLine(&buf);
        color(COL_RESET);
    }

    bg(COL_BG0);
    fg(COL_BG2);

    // Draw squiggles for non-filled lines
    if (editor.numLines < editor.textH)
        for (int i = 0; i < editor.textH - editor.numLines; i++)
        {
            charbufAppend(&buf, "~", 1);
            charbufNextLine(&buf);
        }

    // Draw status line and command line

    bg(COL_FG0);
    fg(COL_BG0);

    char *filename = editor.info.filename;
    charbufAppend(&buf, filename, strlen(filename));
    if (editor.info.dirty && editor.info.fileOpen)
        charbufAppend(&buf, "*", 1);

    bg(COL_BG1);
    fg(COL_FG0);
    charbufNextLine(&buf);

    // Command line

    bg(COL_BG0);
    fg(COL_FG0);

    if (editor.info.hasError)
    {
        fg(COL_RED);
        char *error = editor.info.error;
        charbufAppend(&buf, "error: ", 7);
        charbufAppend(&buf, error, strlen(error));
    }

    charbufNextLine(&buf);
    color(COL_RESET);
    charbufRender(&buf, 0, 0);

    // Show info screen on empty buffer
    if (!editor.info.dirty && !editor.info.fileOpen)
    {
        char *lines[] = {
            TITLE,
            "github.com/jesperkha/wim",
            "last updated "UPDATED,
            "",
            "Editor commands:",
            "exit       ctrl-q / :exit / <escape>",
            "command    ctrl-c                   ",
            "new file   ctrl-n                   ",
            "open file  ctrl-o / :open [filename]",
            "save       ctrl-s / :save           ",
        };

        int numlines = sizeof(lines) / sizeof(lines[0]);
        int y = editor.height/2 - numlines/2;

        screenBufferBg(COL_BG0);
        screenBufferFg(COL_BLUE);

        for (int i = 0; i < numlines; i++)
        {
            if (i == 1)
                screenBufferFg(COL_FG0);
            if (i == 5)
                screenBufferFg(COL_GREY);

            char *text = lines[i];
            int pad = editor.width/2 - strlen(text)/2;
            editorWriteAt(pad, y + i, text);
        }
    }

    // Set cursor pos
    COORD pos = {editor.col - editor.offx + editor.padH, editor.row - editor.offy};
    SetConsoleCursorPosition(editor.hbuffer, pos);
}

// 100% effective for clearing screen. screenBufferClearAll may leave color
// artifacts sometimes, but is much faster.
void renderBufferBlank()
{
    cursorTempPos(0, 0);
    int size = editor.width * editor.height;
    memset(editor.renderBuffer, (int)' ', size);
    screenBufferWrite(editor.renderBuffer, size);
    cursorRestore();
}

// ---------------------- STATUS BAR ----------------------

// Does not update a field if left as NULL.
void statusBarUpdate(char *filename, char *error)
{
    if (filename != NULL)
    {
        // Get files basename
        char *slash = filename;
        for (int i = strlen(filename); i >= 0; i--)
        {
            if (filename[i] == '/' || filename[i] == '\\')
                break;

            slash = filename+i;
        }

        strcpy(editor.info.filename, slash);
        strcpy(editor.info.filepath, filename);
    }

    if (error != NULL)
        strcpy(editor.info.error, error);

    editor.info.hasError = error != NULL;
    renderBuffer();
}

void statusBarClear()
{
    statusBarUpdate(NULL, NULL);
}
