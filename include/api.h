// The high level api for rum commands and actions.

#pragma once

// Writes a single character to buffer if valid.
void TypingWriteChar(char c);
// Writes text after cursor pos.
void TypingWrite(char *source, int length);
// Deletes a single character before the cursor.
void TypingBackspace();
// Deletes n characters before the cursor. Does not delete/wrap lines.
void TypingBackspaceMany(int count);
// Deletes one character to the right.
void TypingDelete();
// Deletes n characters after the cursor. Does not delete/wrap lines.
void TypingDeleteMany(int count);
// Inserts newline while keeping indentation cursor position.
void TypingNewline();
// Deletes line at current cursor pos.
void TypingDeleteLine();
// Inserts tab according to current editor tab size config.
void TypingInsertTab();
// Clears line and inserts correct indent
void TypingClearLine();
// Deletes marked text/lines in visual mode
void TypingDeleteMarked();
// Comments out current line
void TypingCommentOutLine();
// Comments out lines up to and including last row
void TypingCommentOutLines(int from, int to);

void FindPrompt();
// Returns position of first character of next word
int FindNextWordBegin();
// Returns position of first character of previous word
int FindPrevWordBegin();
// Returns first non-blank character in line
int FindLineBegin();
// Returns last non-blank character in line
int FindLineEnd();
// Returns row of next blank line below
int FindNextBlankLine();
// Returns row of prev blank line above
int FindPrevBlankLine();
// Returns position of next or prev c on current line
int FindNextChar(char c, bool backwards);
// Returns next instance of search term in file from current cursor position
CursorPos FindNext(char *search, int length);
// Returns prev instance of search term in file from current cursor position
CursorPos FindPrev(char *search, int length);