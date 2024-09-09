#pragma once

#define MAX_TOKEN_WORD 128

typedef struct SyntaxToken
{
    bool isSymbol;
    bool isWord;
    bool isNumber;
    bool isString;
    bool eof;

    char word[MAX_TOKEN_WORD];
    int wordLength;
    int pos;
} SyntaxToken;

typedef struct LineIterator
{
    const char *line;
    int lineLength;
    int pos;
} LineIterator;

typedef struct HlLine
{
    char *line;    // Line pointer, must not be freed
    int length;    // Length og line with colors (true length)
    int rawLength; // Raw length of line without colors
    int row;       // Row in buffer
} HlLine;

// Creates new iterator to use when looping over tokens in line
LineIterator NewLineIterator(const char *line, int lineLength);

// Gets next 'token' in line. Either an ascii word string or a single symbol.
// Returns token with eof set to true on end of line.
SyntaxToken GetNextToken(LineIterator *iter);

// Returns pointer to highlight buffer. Must NOT be freed. Line is the
// pointer to the line contents and the length is excluding the NULL
// terminator. Writes byte length of highlighted text to newLength.
HlLine ColorLine(Buffer *b, char *line, int lineLength, int row);