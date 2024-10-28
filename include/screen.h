#pragma once

// Renders everything to the terminal. Sets cursor position. Shows welcome screen.
void Render();

// Sets command line error message. NULL for no message.
void SetError(char *error);

void ScreenWrite(char *string, int length);
void ScreenWriteAt(int x, int y, char *text);
void ScreenColor(char *bg, char *fg);
void ScreenColorReset();
void ScreenBg(char *col);
void ScreenFg(char *col);
