#pragma once

// Renders everything to the terminal. Sets cursor position. Shows welcome screen.
void Render();

// Sets status bar info. Passing NULL for filename will leave the current one.
// Passing NULL for error will remove the current error. Call Render to update.
void SetStatus(char *filename, char *error);

// Status codes returned by UI functions.
typedef enum UiStatus
{
    UI_YES,
    UI_NO,
    UI_OK,
    UI_CANCEL,
} UiStatus;

typedef struct UiResult
{
    UiStatus status; // Cancel-like statuses should be respected
    int choice;      // Index of chosen item
    char *buffer;    // Buffer with user input
    int length;      // Length of buffer
    int maxLength;   // Max length set at function call and size allocated for buffer
} UiResult;

void UiFreeResult(UiResult res);
// Prompts command line for yes/no answer and hangs. Can be canceled with ESC.
UiStatus UiPromptYesNo(char *message, bool select);
// Prompts user for text input under status line. Remember to check status and free result.
UiResult UiGetTextInput(char *prompt, int maxSize);
// Prompts user to choose an item from the list. Prompt may be NULL. Remember to check status and free result.
UiResult UiPromptList(char **items, int numItems, char *prompt);
UiResult UiPromptListEx(char **items, int numItems, char *prompt, int startIdx);

void ScreenWrite(const char *string, int length);
void ScreenWriteAt(int x, int y, const char *text);
void ScreenClearLine(int row);
void ScreenClear();
void ScreenColor(char *bg, char *fg);
void ScreenColorReset();
void ScreenBg(char *col);
void ScreenFg(char *col);
