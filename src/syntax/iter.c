#include "rum.h"

LineIterator NewLineIterator(const char *line, int lineLength)
{
    AssertNotNull(line);
    Assert(lineLength >= 0);

    return (LineIterator){
        .line = line,
        .lineLength = lineLength,
        .pos = 0,
    };
}

SyntaxToken GetNextToken(LineIterator *iter)
{
    SyntaxToken tok = {0};

    // Already EOF
    if (iter->pos >= iter->lineLength || iter->lineLength <= 0)
    {
        tok.eof = true;
        return tok;
    }

    char c = iter->line[iter->pos];

    // String
    if (strchr("'\"`", c) != NULL)
    {
        tok.isString = true;
        while (iter->pos < iter->lineLength)
        {
            char c = iter->line[iter->pos];
            tok.word[tok.wordLength++] = c;
            iter->pos++;
            if (tok.wordLength > 1 && strchr("'\"`", c) != NULL)
                return tok;
            if (tok.wordLength == MAX_TOKEN_WORD)
                return tok;
        }

        return tok;
    }

    // Number
    if (isdigit(c))
    {
        tok.isNumber = true;
        while (iter->pos < iter->lineLength)
        {
            char c = iter->line[iter->pos];
            if (!isdigit(c) && c != '.')
                return tok;
            tok.word[tok.wordLength++] = c;
            iter->pos++;
            if (tok.wordLength == MAX_TOKEN_WORD)
                return tok;
        }

        return tok;
    }

    // Multi-char word
    if (isalpha(c) || c == '_')
    {
        tok.isWord = true;
        while (iter->pos < iter->lineLength)
        {
            char c = iter->line[iter->pos];
            if (!isalnum(c) && c != '_')
                return tok;
            tok.word[tok.wordLength++] = c;
            iter->pos++;
            if (tok.wordLength == MAX_TOKEN_WORD)
                return tok;
        }

        return tok;
    }

    // Single char symbol
    if (!isalpha(c))
    {
        tok.isSymbol = true;
        tok.word[0] = c;
        tok.wordLength = 1;
        iter->pos++;
        return tok;
    }

    Assert(false); // Unreachable
}

// Returns true if sequence was found, and also keeps iteration made to iter.
// Otherwise iteration is reset to where it was.
bool MatchSymbolSequence(LineIterator *iter, char *sequence)
{
    int prevPos = iter->pos; // Reset to this if comment not found
    bool matched = true;

    for (int i = 1; i < (int)strlen(sequence); i++)
    {
        SyntaxToken tok = GetNextToken(iter);
        if (tok.eof || !tok.isSymbol || tok.word[0] != sequence[i])
        {
            matched = false;
            break;
        }
    }

    if (!matched)
        iter->pos = prevPos;

    return matched;
}
