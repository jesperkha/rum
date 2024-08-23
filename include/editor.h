#pragma once

#include <windows.h>

#define curBuffer (editor.buffers[editor.activeBuffer])
#define curRow (curBuffer->cursor.row)
#define curCol (curBuffer->cursor.col)
#define curLine (curBuffer->lines[curRow])
#define curChar (curLine.chars[curCol])

// Populates editor global struct and creates empty file buffer. Exits on error.
void EditorInit(CmdOptions options);
void EditorFree();
// Handles inputs for insert mode (default)
Status HandleInsertMode(InputInfo *info);
// Handles inputs for Vim mode (command mode)
Status HandleVimMode(InputInfo *info);
// Sets editor input mode
void EditorSetMode(InputMode mode);
// Waits for input and takes action for insert mode.
Status EditorHandleInput();
// Loads file into buffer. Filepath must either be an absolute path
// or name of a file in the same directory as working directory.
Status EditorOpenFile(char *filepath);
// Writes content of buffer to filepath. Always truncates file.
Status EditorSaveFile();
// Replaces current buffer with b.
void EditorSetCurrentBuffer(Buffer *b);
// Loads help text into a new buffer and displays it.
void EditorShowHelp();
// Returns index of new buffer
int EditorNewBuffer();
// Splits buffers, setting the right to an empty buffer
void EditorSplitBuffers();
void EditorUnsplitBuffers();
// Sets active buffer to given id
void EditorSetActiveBuffer(int idx);
// Swaps the current active buffer with another one
void EditorSwapActiveBuffer(int idx);
// Closes given buffer. Sets active to next available.
void EditorCloseBuffer(int idx);

// Asks user if they want to exit without saving. Writes file if answered yes.
void PromptFileNotSaved(Buffer *b);
// Prompts user for command input. If command is not NULL, it is set as the
// current command and cannot be removed by the user, used for shorthands.
void PromptCommand(char *command);

// Loads config file and writes to given config. Sets default config
// if file failed to open.
Status LoadConfig(Config *config);
// Loads theme data into colors. Returns false on failure.
Status LoadTheme(char *name, Colors *colors);
// Loads syntax from file and sets new table in buffer if found.
Status LoadSyntax(Buffer *b, char *filepath);

// Undos last action if any.
void Undo();
// Saves action to undo stack. May group it with previous actions if suitable.
void UndoSaveAction(Action type, char *text, int textLen);
void UndoSaveActionEx(Action type, int row, int col, char *text, int textLen);
// Joins last n actions under same undo call.
void UndoJoin(int n);