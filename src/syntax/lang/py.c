#include "rum.h"

extern Colors colors;

static char *keywords[] = {
    "False",
    "await",
    "else",
    "import",
    "pass",
    "True",
    "class",
    "finally",
    "is",
    "return",
    "and",
    "continue",
    "for",
    "lambda",
    "try",
    "as",
    "def",
    "from",
    "nonlocal",
    "while",
    "assert",
    "del",
    "global",
    "not",
    "with",
    "async",
    "elif",
    "if",
    "or",
    "yield",
    "break",
    "except",
    "in",
    "raise",
};

static char *types[] = {
    "int",
    "float",
    "str",
    "dict",
    "list",
    "None",
    "bool",
    "complex",
    "tuple",
    "range",
    "set",
    "set",
    "bytes",
};

void langPy(HlLine line, LineIterator *iter, CharBuf *cb)
{
    if (line.length == 0)
        return;

    char *comment = "#";

    while (true)
    {
        SyntaxToken tok = GetNextToken(iter);
        if (tok.eof)
            break;

        char *col = colors.fg0;
        bool colored = false;

        if (tok.isString)
            col = colors.string;
        else if (tok.isNumber || (tok.isSymbol && tok.word[0] == '\\'))
            col = colors.number;
        else if (tok.isWord)
        {
            // Function name
            {
                int prevPos = iter->pos;
                SyntaxToken next = GetNextToken(iter);
                if (next.isSymbol && next.word[0] == '(')
                {
                    col = colors.function;
                    colored = true;
                }
                iter->pos = prevPos;
            }

            // Reserved keyword or type name
            {
                int numKeywords = sizeof(keywords) / sizeof(char *);
                int numTypes = sizeof(types) / sizeof(char *);

                for (int i = 0; i < numKeywords; i++)
                {
                    if (!strcmp(keywords[i], tok.word))
                    {
                        col = colors.keyword;
                        colored = true;
                        break;
                    }
                }

                for (int i = 0; i < numTypes; i++)
                {
                    if (!strcmp(types[i], tok.word))
                    {
                        col = colors.type;
                        colored = true;
                        break;
                    }
                }
            }
        }
        else if (tok.isSymbol)
        {
            char c = tok.word[0];

            // .foo
            if (c == '.')
            {
                int prevPos = iter->pos;
                SyntaxToken next = GetNextToken(iter);
                if (next.isWord)
                {
                    CbColorWord(cb, colors.bracket, ".", 1);
                    CbColorWord(cb, colors.object, next.word, next.wordLength);
                    continue;
                }
                iter->pos = prevPos;
            }

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

            // Decorators
            if (c == '@')
            {
                CbColorWord(cb, colors.bracket, tok.word, tok.wordLength);
                tok = GetNextToken(iter); // Decorator name
                CbColorWord(cb, colors.symbol, tok.word, tok.wordLength);
                continue;
            }

            // Normal symbols
            if (!colored)
            {
                if (strchr("()[]{};,:", c) != NULL)
                    col = colors.bracket;
                else if (strchr("+-/*=~%<>&|?!^", c) != NULL)
                    col = colors.symbol;
            }
        }

        CbColorWord(cb, col, tok.word, tok.wordLength);
    }
}