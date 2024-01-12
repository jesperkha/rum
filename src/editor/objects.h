#pragma once

#include <windows.h>

#define COLORS_LENGTH 144      // Size of editor.colors
#define SYNTAX_NAME_LEN 16     // Length of extension name in syntax file
#define BUFFER_LINE_CAP 32     // Editor line array cap
#define DEFAULT_LINE_LENGTH 32 // Length of line char array
#define THEME_NAME_LEN 32      // Length of name in theme file

enum FunctionStatusCode
{
    RETURN_ERROR,
    RETURN_SUCCESS,
};

typedef enum FunctionStatusCode Status;

// State for editor. Contains information about the current session.
typedef struct Info
{
    char filename[64];
    char filepath[260];
    char error[64];
    int fileType;
    bool hasError;
    bool dirty;
    bool fileOpen;
    bool syntaxReady;
} Info;

// File types for highlighting, set to Info.fileType
enum FileTypes
{
    FT_UNKNOWN,
    FT_C,
    FT_PYTHON,
};

// Config loaded from file, considered read only. Can be temporarily changed at runtime.
typedef struct Config
{
    bool syntaxEnabled;
    bool matchParen;
    bool useCRLF;
    int tabSize;
} Config;

// Line in editor line array
typedef struct Line
{
    int row;
    int cap;
    int length;
    char *chars;
} Line;

typedef struct Editor
{
    Info info;
    Config config;

    HANDLE hstdin;  // Handle for standard input
    HANDLE hbuffer; // Handle to new screen buffer

    COORD initSize;    // Inital size to return to
    int width, height; // Size of terminal window
    int textW, textH;  // Size of text editing area
    int padV, padH;    // Vertical and horizontal padding

    // Cursor

    int row, col;           // Current row and col of cursor in buffer
    int colMax;             // Keep track of the most right pos of the cursor when moving down
    int offx, offy;         // x, y offset from left/top
    int indent;             // Indent in spaces for current line
    int scrollDx, scrollDy; // Minimum distance from top/bottom or left/right before scrolling

    // Buffer

    int numLines;
    int lineCap;
    Line *lines;

    char *renderBuffer;         // Written to and printed on render
    char colors[COLORS_LENGTH]; // Theme colors

    // Table used to store syntax information for current file type
    struct syntaxTable
    {
        char ext[SYNTAX_NAME_LEN];
        char syn[2][1024];
        int len[2];
    } syntaxTable;
} Editor;

// Different event types for InputInfo
enum InputEvents
{
    INPUT_UNKNOWN,
    INPUT_KEYDOWN,
    INPUT_WINDOW_RESIZE,
};

// Input record with curated input information.
typedef struct InputInfo
{
    enum InputEvents eventType;
    char asciiChar;
    int keyCode;
    bool ctrlDown;
} InputInfo;

enum KeyCodes
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
};