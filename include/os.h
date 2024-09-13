#pragma once

void *MemAlloc(int size);
void *MemZeroAlloc(int size);
void *MemRealloc(void *ptr, int newSize);
void MemFree(void *ptr);

// Read file realitive to cwd. Writes to size. Returns null on failure. Free content pointer.
char *OsReadFile(const char *filepath, int *size);
// Truncates file or creates new one if it doesnt exist. Returns true on success.
bool OsWriteFile(const char *filepath, char *data, int size);

// Hangs when waiting for input. Returns error if read failed. Writes to info.
Status ReadTerminalInput(InputInfo *info);

// Creates and sets new terminal buffer
// Updates buffer size to terminal size
// Sets raw input mode
// Sets terminal title
// Disables cursor blinking
void TermInit(Editor *editor);

// Updates terminal buffer size to fill windows. Sets values to editor.
void TermUpdateSize(Editor *editor);

// Sets cursor position in terminal
void TermSetCursorPos(int x, int y);
void TermSetCursorVisible(bool visible);
void TermWrite(char *string, int length);