// The high level api for wim commands and actions.

#pragma once

// Writes a single character to buffer if valid.
void TypingWriteChar(char c);

// Writes text after cursor pos.
void TypingWrite(char *source, int length);

// Deletes a single character before the cursor.
void TypingBackspace();

// Deletes one character to the right.
void TypingDelete();

// Inserts newline while keeping indentation cursor position.
void TypingNewline();

// Deletes line at current cursor pos.
void TypingDeleteLine();

// Inserts tab according to current editor tab size config.
void TypingInsertTab();
