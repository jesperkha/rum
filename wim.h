#pragma once

#define RETURN_SUCCESS 0
#define RETURN_ERROR -1

// Corresponds to a single line, or row, in the editor buffer
typedef struct line
{
    int idx;      // Row index in file
    int size;     // Length of line
    int rsize;    // Length of rendered string
    char *chars;  // Characters in line
    char *render; // Points to beginning of rendered chars in chars
} line;
