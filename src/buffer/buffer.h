#pragma once

// Writes characters to buffer at cursor pos. Does not filter out non-writable characters.
void BufferWrite(char *source, int length);

// Deletes the caharcter before the cursor position. Deletes line if cursor is at beginning.
void BufferDeleteChar();

// Inserts new line at row. If row is -1 line is appended to end of file.
void BufferInsertLine(int row);

// Deletes line at row and move all lines below upwards.
void BufferDeleteLine(int row);

// Copies and removes all characters behind the cursor position,
// then pastes them at the end of the line below.
void BufferSplitLineDown(int row);

// Moves line content from row to end of line above.
void BufferSplitLineUp(int row);

// Scrolls buffer vertically by delta y.
void BufferScroll(int dy);

// Shorthands for scrolling up and down by one. Moves cursor too.
void BufferScrollDown();
void BufferScrollUp();

// Inserts tab according to current editor tab size config.
void TypingInsertTab();

// Matches braces, parens, strings etc. Also removes extra closing brackets
// when typing them out back to back, eg. ()
void TypingMatchParen(char c);

// Moves paren down and indents line when pressing enter after a paren.
void TypingBreakParen();

// Deletes one character to the right.
void TypingDeleteForward();

#define COL_RESET "\x1b[0m"

// Index order is alphabetical
#define COL_BG0 (12 * 1)     // Editor background
#define COL_BG1 (12 * 2)     // Statusbar and current line bg
#define COL_BG2 (12 * 3)     // Comments, line numbers
#define COL_FG0 (12 * 5)     // Text
#define COL_AQUA (12 * 0)    // Math symbol, macro
#define COL_BLUE (12 * 4)    // Object
#define COL_GREY (12 * 6)    // Other symbol
#define COL_GREEN (12 * 7)   // String, char
#define COL_ORANGE (12 * 8)  // Type name
#define COL_PINK (12 * 9)    // Number
#define COL_RED (12 * 10)    // Keyword
#define COL_YELLOW (12 * 11) // Function name

// Returns pointer to highlight buffer. Must NOT be freed. Line is the
// pointer to the line contents and the length is excluding the NULL
// terminator. Writes byte length of highlighted text to newLength.
char *HighlightLine(char *line, int lineLength, int *newLength);

// Used to store text before rendering.
typedef struct CharBuf
{
    char *buffer;
    char *pos;
    int lineLength;
} CharBuf;

// Returns pointer to empty CharBuf mapped to input buffer.
CharBuf *CbNew(char *buffer);

// Resets buffer to starting state. Does not memclear the internal buffer.
void CbReset(CharBuf *buf);

void CbAppend(CharBuf *buf, char *src, int length);

// Fills remaining line with space characters based on editor width.
void CbNextLine(CharBuf *buf);

// Adds background and foreground color to buffer.
void CbColor(CharBuf *buf, int bg, int fg);
void CbBg(CharBuf *buf, int bg);
void CbFg(CharBuf *buf, int fg);

// Adds COL_RESET to buffer
void CbColorReset(CharBuf *buf);

// Prints buffer at x, y with accumulated length only.
void CbRender(CharBuf *buf, int x, int y);

typedef enum Action
{
    A_WRITE,
    A_DELETE,
    A_DELETE_LINE,
    A_INSERT_LINE,
} Action;

void UndoStackInit();
void UndoStackFree();

// Adds action to stack.
void AppendEditAction(Action type, int row, int col, char *text);

// Undoes last action.
void Undo();

// Redoes last undone action.
void Redo();