#pragma once

typedef struct CursorPos
{
    int row;
    int col;
} CursorPos;

// Editor configuration loaded from config file. Editor must be reloaded for all
// changes to take effect. Config is global and affects all buffers.
typedef struct Config
{
    bool syntaxEnabled; // Enable syntax highlighting for some files
    bool matchParen;    // Match ending parens when typing. eg: '(' adds a ')'
    bool useCRLF;       // Use CRLF line endings. (NOT IMPLEMENTED)
    byte tabSize;       // Amount of spaces a tab equals

    // Cmd config

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
} Action;

#define EDITOR_ACTION_BUFSIZE 128 // Character cap for string in action

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
    char *longText;
} EditorAction;

#define COLOR_SIZE 13 // Size of a color string including NULL

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
    int row, col;   // Row/col in raw text buffer
    int colMax;     // Furthest right position while moving up/down
    int indent;     // Of current line
    int offx, offy; // Inital offset from left/top
    int scrollDx;   // Minimum distance before scrolling right
    int scrollDy;   // Minimum distance before scrolling up/down
} Cursor;

#define LINE_DEFAULT_LENGTH 32

// Line in buffer. Holds raw text.
typedef struct Line
{
    int row;
    int cap;
    int length;
    int indent; // Updated on cursor movement
    char *chars;
} Line;

// All filetypes recognized by the editor and
// with syntax highlighting available.
typedef enum FileType
{
    FT_UNKNOWN,
    FT_C,
    FT_PYTHON,
} FileType;

#define SYNTAX_COMMENT_SIZE 8
#define FILE_EXTENSION_SIZE 16

// Table used to store syntax information for current file type
typedef struct SyntaxTable
{
    char comment[SYNTAX_COMMENT_SIZE];
    char extension[FILE_EXTENSION_SIZE]; // File extension
    int numWords[2];                     // Number of words in words

    // Null seperated list of words. First is keywords, second types.
    char words[2][1024];
} SyntaxTable;

#define BUFFER_DEFAULT_LINE_CAP 32
#define MAX_SEARCH 64

// A buffer holds text, usually a file, and is editable.
typedef struct Buffer
{
    Cursor cursor;
    SyntaxTable *syntaxTable;

    bool isFile;      // Does the buffer contain a file?
    bool dirty;       // Has the buffer changed since last save?
    bool syntaxReady; // Is syntax highlighting available for this file?
    bool readOnly;    // Is file read-only? Default for non-file buffers like help.

    char filepath[PATH_MAX]; // Full path to file
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
    EditorAction *undos; // List pointer

    bool highlight;
    CursorPos hlBegin;
    CursorPos hlEnd;
} Buffer;

typedef enum InputMode
{
    MODE_INSERT,
    MODE_EDIT,   // Vim/edit command mode
    MODE_CUSTOM, // Defined by config (todo)
} InputMode;

#define EDITOR_BUFFER_CAP 16
#define MAX_PADDING 512

// The Editor contains the buffers and the current state of the editor.
typedef struct Editor
{
    int width, height; // Total size of editor

    HANDLE hbuffer; // Handle for created screen buffer
    HANDLE hstdout; // NOT USED
    HANDLE hstdin;  // Console input

    InputMode mode;

    int numBuffers;
    int activeBuffer; // The buffer currently in focus
    bool splitBuffers;
    int leftBuffer;  // Always set
    int rightBuffer; // Only set if splitBuffers is true
    bool uiOpen;
    Buffer *buffers[EDITOR_BUFFER_CAP];

    char *renderBuffer;
    char padBuffer[MAX_PADDING]; // Region filled with space characters
} Editor;