// Defines a set of methods used to interact with the editor without
// needing to know how it works (to a certain degree), hence the name
// wimapi. This is what should be used when writing extensions and new
// high level features for the editor.

#pragma once

#include <stdbool.h>

enum EventType
{
    KEYDOWN,
    WINDOW_RESIZE,
};

// Input information passed to onInput function.
typedef struct WimInputRecord
{
    enum EventType eventType;
    char asciiChar;
    int keyCode;
    bool ctrlDown;
} WimInputRecord;

// Prevents the default action from happening, eg. prevent a character
// from being typed from onInput(). Only has effect when called from an
// event function defined in event.c
void PreventDefault();

// Writes characters to buffer starting at the current cursor position.
// Does not filter non-ascii values. Length is the length of the input
// string EXCLUDING the NULL terminator.
void BufferWrite(char *source, int length);