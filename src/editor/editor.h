#pragma once

#include <windows.h>
#include "objects.h"

// Populates editor global struct and creates empty file buffer. Exits on error.
void EditorInit(CmdOptions options);
void EditorExit();

// Reset editor to empty file buffer. Resets editor Info struct.
void EditorReset();

// Hangs when waiting for input. Returns error if read failed. Writes to info.
Status EditorReadInput(InputInfo *info);

// Waits for input and takes action for insert mode.
Status EditorHandleInput();

// Loads file into buffer. Filepath must either be an absolute path
// or name of a file in the same directory as working directory.
Status EditorOpenFile(char *filepath);

// Writes content of buffer to filepath. Always truncates file.
Status EditorSaveFile();

// Prompts user for command input. If command is not NULL, it is set as the
// current command and cannot be removed by the user, used for shorthands.
void EditorPromptCommand(char *command);

// Reads theme file and sets colorscheme if found.
Status EditorLoadTheme(char *theme);

// Loads syntax for given file extension, omitting the period.
// Writes to editor.syntaxTable struct, used by highlight function.
Status EditorLoadSyntax(char *extension);

void Undo();
void Redo();

// Saves action to undo stack. May group it with previous actions if suitable.
void UndoSaveAction(Action type, char *text, int textLen);
void UndoSaveActionEx(Action type, int row, int col, char *text, int textLen);

// Joins last n actions under same undo call.
void UndoJoin(int n);
