#pragma once

typedef struct String
{
    bool null;
    int length;
    char *s;
} String;

#define STRING(str, len) ((String){.s = str, .length = len})
#define NULL_STRING ((String){.length = 0, .null = true, .s = NULL})

typedef struct CursorPos
{
    int row;
    int col;
} CursorPos;

typedef unsigned char byte;

// Editor configuration loaded from config file. Editor must be reloaded for all
// changes to take effect. Config is global and affects all buffers.
typedef struct Config
{
    bool syntaxEnabled;         // Enable syntax highlighting for some files
    bool matchParen;            // Match ending parens when typing. eg: '(' adds a ')'
    bool useCRLF;               // Use CRLF line endings. (NOT IMPLEMENTED)
    byte tabSize;               // Amount of spaces a tab equals
    char theme[THEME_NAME_LEN]; // Default theme

    // Set by command line options

    bool rawMode; // No colors
} Config;

// Action types for undo to keep track of which actions to group.
typedef enum Action
{
    A_JOIN,        // Join multiple actions into larger undo
    A_UNDO,        // Editor undo
    A_CURSOR,      // Set cursor pos (for delete)
    A_WRITE,       // Write text
    A_DELETE,      // Delete text forward
    A_DELETE_BACK, // Same as backspace, but without joining etc
    A_BACKSPACE,   // Delete backwards, reverses on paste
    A_DELETE_LINE, // Delete line only
    A_INSERT_LINE, // Insert line only
    A_OVERWRITE,   // Overwriting text
} Action;

// Object representing an executable action by the editor (write, delete, etc).
typedef struct EditorAction
{
    Action type;
    int numUndos; // Used for join
    int row;
    int col;
    int endCol;
    int textLen;
    char text[EDITOR_ACTION_BUFSIZE];
    bool isLongText;
    // When deleting the first and only line the undo should not add a new line
    // when pasting the text back, but rather just write it to the empty line
    bool noNewline;
    char *longText;
} EditorAction;

// Dynamic list of actions
typedef struct UndoList
{
    int length;
    int cap;
    EditorAction *undos;
} UndoList;

#define COL_RESET "\x1b[0m"
#define COL_HL "135;138;000"

// The editor keeps a single instance of this struct globally available
// to easily get color values from a loaded theme.
typedef struct Colors
{
    char name[32];
    char bg0[COLOR_SIZE];      // Editor background
    char bg1[COLOR_SIZE];      // Statusbar and current line bg
    char bg2[COLOR_SIZE];      // Comments, line numbers
    char fg0[COLOR_SIZE];      // Text
    char symbol[COLOR_SIZE];   // Math symbol, macro
    char object[COLOR_SIZE];   // Object
    char bracket[COLOR_SIZE];  // Other symbol
    char number[COLOR_SIZE];   // Number
    char string[COLOR_SIZE];   // String, char
    char type[COLOR_SIZE];     // Type name
    char keyword[COLOR_SIZE];  // Keyword
    char function[COLOR_SIZE]; // Function name
    char userType[COLOR_SIZE]; // User defined type/macro
} Colors;

// Event types for InputInfo object.
typedef enum InputEventType
{
    INPUT_UNKNOWN,
    INPUT_KEYDOWN,
    INPUT_WINDOW_RESIZE,
} InputEventType;

// Keycodes recognized by the editor and part of the InputInfo struct.
typedef enum KeyCode
{
    K_BACKSPACE = 8,
    K_TAB = 9,
    K_ENTER = 13,
    K_CTRL = 17,
    K_ESCAPE = 27,
    K_SPACE = 32,
    K_PAGEUP = 33,
    K_PAGEDOWN = 34,
    K_DELETE = 46,
    K_COLON = 58,

    K_ARROW_LEFT = 37,
    K_ARROW_UP,
    K_ARROW_RIGHT,
    K_ARROW_DOWN,
} KeyCode;

// Simplified input record from INPUT_RECORD struct.
typedef struct InputInfo
{
    InputEventType eventType;
    KeyCode keyCode;
    char asciiChar;
    bool ctrlDown;
} InputInfo;

// Each buffer has a cursor object attached to it. The cursor essentially holds
// a pointer to the text to be edited in the Buffer as well as were to place the
// cursor on-screen.
typedef struct Cursor
{
    bool visible;
    int row, col;   // Row/col in raw text buffer
    int colMax;     // Furthest right position while moving up/down
    int indent;     // Of current line
    int offx, offy; // Inital offset from left/top
    int scrollDx;   // Minimum distance before scrolling right
    int scrollDy;   // Minimum distance before scrolling up/down
} Cursor;

// Line in buffer. Holds raw text.
typedef struct Line
{
    int row;
    int cap;
    int length;
    int indent; // Updated on cursor movement
    char *chars;

    bool isMarked; // By search
    int hlStart;
    int hlEnd;

    // These fields are used when a buffer is open as directory in the explorer
    bool isPath;  // Is this a directory entry in explorer?
    bool isDir;   // Is the path to a directory or file?
    int exPathId; // Id to StrArray in buffer with the filename
} Line;

// All filetypes recognized by the editor and
// with syntax highlighting available.
typedef enum FileType
{
    FT_UNKNOWN,
    FT_C,
    FT_PYTHON,
    FT_JSON,
} FileType;

// A buffer holds text, usually a file, and is editable.
typedef struct Buffer
{
    Cursor cursor;

    bool isFile;   // Does the buffer contain a file?
    bool isDir;    // Is this a folder open in the explorer?
    bool dirty;    // Has the buffer changed since last save?
    bool readOnly; // Is file read-only? Default for non-file buffers like help.

    // Set to true if a loaded file uses tabs. Rum always uses spaces for indentation
    // but will convert spaces to tabs when saving and vice versa when loading a file.
    bool useTabs;

    char filepath[MAX_PATH]; // Full path to file
    FileType fileType;

    char search[MAX_SEARCH]; // Current search word
    int searchLen;

    int id; // index of buffer in array
    int textH;
    int padX, padY;    // Padding on left and top of text area
    int offX;          // If this is a right-split buffer, offX is the left edge
    int width, height; // Full size of buffer when rendered

    int numLines;
    int lineCap;
    Line *lines;
    UndoList undos;

    bool showHighlight;
    bool showMarkedLines;
    bool showCurrentLineMark;

    // Highlight points, from a to b
    CursorPos hlA;
    CursorPos hlB;

    StrArray exPaths; // File explorer paths in order
} Buffer;

typedef enum InputMode
{
    MODE_INSERT,      // Regular typing. Control inputs still apply
    MODE_EDIT,        // Vim/edit command mode
    MODE_VISUAL,      // Vim visual/highlight mode
    MODE_VISUAL_LINE, // Same as visual but for whole lines only
    MODE_CUSTOM,      // Defined by config (todo)
    MODE_EXPLORE,     // Separate controls for navigating files
} InputMode;

// The Editor contains the buffers and the current state of the editor.
typedef struct Editor
{
    HANDLE hbuffer; // Handle for created screen buffer
    HANDLE hstdout; // NOT USED
    HANDLE hstdin;  // Console input

    int width, height; // Total size of editor
    int numBuffers;    // Number of open buffers
    int activeBuffer;  // The buffer currently in focus
    int leftBuffer;    // Always set
    int rightBuffer;   // Only set if splitBuffers is true

    bool splitBuffers;
    bool uiOpen; // Is some UI open? Hide buffer contents

    Buffer *buffers[EDITOR_BUFFER_CAP];
    InputMode mode;

    char *renderBuffer;              // Written to before flushing to terminal
    char padBuffer[PAD_BUFFER_SIZE]; // Region filled with space characters
} Editor;