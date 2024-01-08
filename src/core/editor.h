#pragma once

#include <windows.h>

#define COLORS_LENGTH 144  // Size of editor.colors
#define SYNTAX_NAME_LEN 16 // Length of extension name in syntax file

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

// Config loaded from file, considered read only. Can be temporarily changed at runtime.
typedef struct Config
{
    bool syntaxEnabled;
    bool matchParen;
    bool useCRLF;
    int tabSize;
} Config;

typedef struct Line
{
    int idx; // Row index in file, not buffer
    int row; // Relative row in buffer, not file

    int cap;     // Capacity of line
    int length;  // Length of line
    char *chars; // Characters in line
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

    // Todo: move cursor info to seperate struct
    int row, col;           // Current row and col of cursor in buffer
    int colMax;             // Keep track of the most right pos of the cursor when moving down
    int offx, offy;         // x, y offset from left/top
    int indent;             // Indent in spaces for current line
    int scrollDx, scrollDy; // Minimum distance from top/bottom or left/right before scrolling

    int numLines, lineCap; // Count and capacity of lines in array
    Line *lines;           // Array of lines in buffer

    char *renderBuffer; // Written to and printed on render
    char colors[COLORS_LENGTH]; // Theme colors

    // Table used to store syntax information for current file type
    struct syntaxTable
    {
        char ext[SYNTAX_NAME_LEN];
        char syn[2][1024];
        int  len[2];
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

// Populates editor global struct and creates empty file buffer. Exits on error.
void EditorInit();

void editorReset();
void editorExit();
void editorWriteAt(int x, int y, const char *text);
void editorUpdateSize();
int editorReadInput(InputInfo *info);
int editorHandleInput();
void editorPromptFileNotSaved();
int editorOpenFile(char *filepath);
int editorSaveFile();
void editorCommand(char *command);
int editorLoadTheme(const char *theme);
int editorLoadSyntax(const char *extension);