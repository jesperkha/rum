#pragma once

// Writes characters to buffer at cursor pos. Does not filter out non-writable characters.
void BufferWrite(char *source, int length);

// Deletes the character before the cursor position.
void BufferDeleteChar();

// Delete count amount of characters. Does not delete newlines.
void BufferDelete(int count);

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

// Returns number of spaces before the cursor
int BufferGetIndent();

// Writes a single character to buffer if valid.
void TypingWriteChar(char c);

// Deletes a single character before the cursor.
void TypingDeleteChar();

// Inserts newline while keeping indentation cursor position.
void TypingNewline();

// Deletes line at current cursor pos.
void TypingDeleteLine();

// Inserts tab according to current editor tab size config.
void TypingInsertTab();

// Matches braces, parens, strings etc. Also removes extra closing brackets
// when typing them out back to back, eg. ()
void TypingMatchParen(char c);

// Moves paren down and indents line when pressing enter after a paren.
void TypingBreakParen();

// Deletes one character to the right.
void TypingDeleteForward();

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
void CbColor(CharBuf *buf, char *bg, char *fg);
void CbBg(CharBuf *buf, char *bg);
void CbFg(CharBuf *buf, char *fg);

// Adds COL_RESET to buffer
void CbColorReset(CharBuf *buf);

// Prints buffer at x, y with accumulated length only.
void CbRender(CharBuf *buf, int x, int y);
