#include "rum.h"

extern Colors colors;

void langJson(HlLine line, LineIterator *iter, CharBuf *cb)
{
    if (line.length == 0)
        return;

    char *comment = "//";

    while (true)
    {
        SyntaxToken tok = GetNextToken(iter);
        if (tok.eof)
            break;

        char *col = colors.fg0;

        if (tok.isString)
            col = colors.string;
        else if (tok.isNumber || (tok.isSymbol && tok.word[0] == '\\'))
            col = colors.number;
        else if (tok.isWord)
            col = colors.keyword;
        else if (tok.isSymbol)
        {
            char c = tok.word[0];

            // Single line comment
            if (c == comment[0])
            {
                int startPos = iter->pos - 1;
                if (MatchSymbolSequence(iter, comment))
                {
                    char *lineText = (char *)(iter->line + startPos);
                    int length = iter->lineLength - startPos;
                    CbColorWord(cb, colors.bg2, lineText, length);
                    break;
                }
            }
            else
                col = colors.bracket;
        }

        CbColorWord(cb, col, tok.word, tok.wordLength);
    }
}