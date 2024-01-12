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

// Todo: make new console write functions
void screenBufferWrite(const char *string, int length);
void screenBufferClearAll();
void screenBufferBg(int col);
void screenBufferFg(int col);
void screenBufferClearLine(int row);
