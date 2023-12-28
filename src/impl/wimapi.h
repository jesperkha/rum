// Defines a set of methods used to interact with the editor without
// needing to know how it works (to a certain degree), hence the name
// wimapi. This is what should be used when writing extensions and new
// high level features for the editor.

#pragma once

// *** BUFFER ***
// Functions used to manipulate the text buffer containing the raw file data:

// Writes characters to buffer starting at the current cursor position.
// Does not filter non-ascii values. Length is the length of the input
// string EXCLUDING the NULL terminator.
void wimBufferWrite(char *source, int length);
