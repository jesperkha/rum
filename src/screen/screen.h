#pragma once

// Renders everything to the terminal. Sets cursor position. Shows welcome screen.
void Render();

// Sets status bar info. Passing NULL for filename will leave the current one.
// Passing NULL for error will remove the current error. Call Render to update.
void SetStatus(char *filename, char *error);

// Status codes returned by UI functions.
enum UiStatusCode
{
    UI_YES,
    UI_NO,
    UI_OK,
    UI_CANCEL,
};

typedef enum UiStatusCode UiStatus;

// Prompts command line for yes/no answer and hangs. Can be canceled with ESC.
UiStatus UiPromptYesNo(char *message, bool select);

// Prompts command line for text input and hangs. Can be canceled with ESC. Takes
// a pointer to a buffer that receives the text input, size is the size of said
// buffer. If the buffer already contains text it is displayed and made immutable.
UiStatus UiTextInput(int x, int y, char *buffer, int size);

void ScreenWrite(const char *string, int length);
void ScreenWriteAt(int x, int y, const char *text);
void ScreenClearLine(int row);
void ScreenClear();
void ScreenColor(char *bg, char *fg);
void ScreenColorReset();
void ScreenBg(char *col);
void ScreenFg(char *col);
