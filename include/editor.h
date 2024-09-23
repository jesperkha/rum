#pragma once

#define curBuffer (editor.buffers[editor.activeBuffer])
#define curRow (curBuffer->cursor.row)
#define curCol (curBuffer->cursor.col)
#define curLine (curBuffer->lines[curRow])
#define curChar (curLine.chars[curCol])

// Populates editor global struct and creates empty file buffer. Exits on error.
void EditorInit(CmdOptions options);
void EditorFree();
// Sets editor input mode
void EditorSetMode(InputMode mode);
// Waits for input and takes action for insert mode.
Error EditorHandleInput();
// Loads file into buffer. Filepath must either be an absolute path
// or name of a file in the same directory as working directory.
Error EditorOpenFile(char *filepath);
// Writes content of buffer to filepath. Always truncates file.
Error EditorSaveFile();
// Replaces current buffer with b. Sets the id of curbuf to b. Prompt file not saved.
void EditorReplaceCurrentBuffer(Buffer *b);
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
// Opens tab selection menu
void EditorPromptTabSwap();
// Hangs when waiting for input. Returns error if read failed. Writes to info.
Error EditorReadInput(InputInfo *info);
// Opens a new read-only folder buffer in the same directory as the current open file,
// or the workspace root if no file is open. Sets the input mode to MODE_EXPLORE.
void EditorOpenFileExplorer();
void EditorOpenFileExplorerEx(char *directory);

// Returns true if action was performed and normal input handling should be skipped.
bool HandleCtrlInputs(InputInfo *info);
// Handles inputs for insert mode (default)
Error HandleInsertMode(InputInfo *info);
// Handles inputs for Vim mode (command mode)
Error HandleVimMode(InputInfo *info);
// Handles inputs for visual/highlight mode
Error HandleVisualMode(InputInfo *info);
// Handles inputs for visual/highlight line mode
Error HandleVisualLineMode(InputInfo *info);
// Handles input for navigating files
Error HandleExploreMode(InputInfo *info);

// Asks user if they want to exit without saving. Writes file if answered yes.
void PromptFileNotSaved(Buffer *b);
// Prompts user for command input. If command is not NULL, it is set as the
// current command and cannot be removed by the user, used for shorthands.
void PromptCommand(char *command);

// Loads config file and writes to given config. Sets default config
// if file failed to open.
Error LoadConfig(Config *config);
// Loads theme data into colors. Returns false on failure.
Error LoadTheme(char *name, Colors *colors);
// Loads syntax from file and sets new table in buffer if found.
Error LoadSyntax(Buffer *b, char *filepath);
// Looks for files in the directory of the executable, eg. config, runtime etc.
// Returns pointer to file data, NULL on error. Writes to size. Remember to free!
char *ReadConfigFile(const char *file, int *size);

UndoList UndoNewList();
// Undos last action if any.
void Undo();
// Saves action to undo stack. May group it with previous actions if suitable.
void UndoSaveAction(Action type, char *text, int textLen);
void UndoSaveActionEx(Action type, int row, int col, char *text, int textLen);
// Joins last n actions under same undo call.
void UndoJoin(int n);

// Updates terminal buffer size to fill windows. Sets values to editor.
void TermUpdateSize();
// Sets cursor position in terminal
void TermSetCursorPos(int x, int y);
void TermSetCursorVisible(bool visible);
void TermWrite(char *string, int length);