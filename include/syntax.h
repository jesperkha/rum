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
    bool isCurrentLine;
} HlLine;

// Creates new iterator to use when looping over tokens in line
LineIterator NewLineIterator(const char *line, int lineLength);

// Gets next 'token' in line. Either an ascii word string or a single symbol.
// Returns token with eof set to true on end of line.
SyntaxToken GetNextToken(LineIterator *iter);

// Returns true if sequence was found, and also keeps iteration made to iter.
// Otherwise iteration is reset to where it was.
bool MatchSymbolSequence(LineIterator *iter, char *sequence);

// Returns pointer to highlight buffer. Must NOT be freed. Line is the
// pointer to the line contents and the length is excluding the NULL
// terminator. Writes byte length of highlighted text to newLength.
HlLine ColorLine(Buffer *b, HlLine line);

// Adds highlight to marked areas and returns new line pointer.
HlLine HighlightLine(Buffer *b, HlLine line);

// Marks part of line for things like search. Only call if buffer line enables it.
HlLine MarkLine(HlLine line, int start, int end);

// Language highlighters

void langC(HlLine line, LineIterator *iter, CharBuf *cb);
void langPy(HlLine line, LineIterator *iter, CharBuf *cb);
void langJson(HlLine line, LineIterator *iter, CharBuf *cb);