// The high level api for wim commands and actions.

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

// Returns position of first character of next word
int FindNextWordBegin();
// Returns position of first character of previous word
int FindPrevWordBegin();
// Returns first non-blank character in line
int FindLineBegin();
// Returns last non-blank character in line
int FindLineEnd();
// Returns position of next or prev c on current line
int FindNextChar(char c, bool backwards);