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

// Creates new iterator to use when looping over tokens in line
LineIterator NewLineIterator(const char *line, int lineLength);

// Gets next 'token' in line. Either an ascii word string or a single symbol.
// Returns token with eof set to true on end of line.
SyntaxToken GetNextToken(LineIterator *iter);