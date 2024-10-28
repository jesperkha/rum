#include "rum.h"

extern Colors colors;

static char *keywords[] = {
    "auto",
    "break",
    "case",
    "continue",
    "default",
    "do",
    "else",
    "enum",
    "extern",
    "for",
    "goto",
    "if",
    "register",
    "return",
    "sizeof",
    "static",
    "struct",
    "switch",
    "typedef",
    "union",
    "volatile",
    "while",
    "NULL",
    "true",
    "false",
};

static char *types[] = {
    "int",
    "long",
    "double",
    "float",
    "char",
    "unsigned",
    "signed",
    "void",
    "short",
    "auto",
    "const",
    "bool",
};

void langC(HlLine line, LineIterator *iter, CharBuf *cb)
{
    char *comment = "//";
    char *blockCommentStart = "/*";
    char *blockCommentEnd = "*/";
    static int blockCommentDepth = 0;

    bool isIncludeMacro = false;

    if (line.row == 0) // Reset for new render
        blockCommentDepth = 0;

    while (true)
    {
        SyntaxToken tok = GetNextToken(iter);
        if (tok.eof)
            break;

        char *col = colors.fg0;
        bool colored = false;

        // Color everything grey until block comment ends
        if (blockCommentDepth > 0)
        {
            if (tok.isSymbol)
            {
                // Block comment end
                char c = tok.word[0];

                if (c == blockCommentStart[0] && MatchSymbolSequence(iter, blockCommentStart))
                {
                    CbColorWord(cb, colors.bg2, blockCommentStart, strlen(blockCommentStart));
                    blockCommentDepth++;
                    continue;
                }

                if (c == blockCommentEnd[0] && MatchSymbolSequence(iter, blockCommentEnd))
                {
                    CbColorWord(cb, colors.bg2, blockCommentEnd, strlen(blockCommentEnd));
                    blockCommentDepth--;
                    continue;
                }
            }
            CbColorWord(cb, colors.bg2, tok.word, tok.wordLength);
            continue;
        }

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

            // C user types (just checks first letter is capitalized or two words follow eachother)
            if (!colored && isupper(tok.word[0]))
            {
                col = colors.userType;
                colored = true;
            }
            else if (!colored)
            {
                int prevPos = iter->pos;
                SyntaxToken space = GetNextToken(iter);
                SyntaxToken next = GetNextToken(iter);
                if (space.word[0] == ' ' && next.isWord)
                {
                    col = colors.userType;
                    colored = true;
                }
                iter->pos = prevPos;
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

            // ->foo
            if (c == '-')
            {
                int prevPos = iter->pos;
                SyntaxToken next = GetNextToken(iter);
                SyntaxToken word = GetNextToken(iter);
                if (next.isSymbol && next.word[0] == '>' && word.isWord)
                {
                    CbColorWord(cb, colors.bracket, "->", 2);
                    CbColorWord(cb, colors.object, word.word, word.wordLength);
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

            // Block comment begin
            if (c == blockCommentStart[0] && MatchSymbolSequence(iter, blockCommentStart))
            {
                CbColorWord(cb, colors.bg2, blockCommentStart, strlen(blockCommentStart));
                blockCommentDepth++;
                continue;
            }

            // C macros
            if (c == '#')
            {
                CbColorWord(cb, colors.bracket, tok.word, tok.wordLength);
                tok = GetNextToken(iter); // Macro type name as well
                CbColorWord(cb, colors.symbol, tok.word, tok.wordLength);
                if (!strcmp(tok.word, "include"))
                    isIncludeMacro = true;
                continue;
            }

            // Normal symbols
            if (!colored)
            {
                // Stringify <foo.h>
                if (isIncludeMacro && tok.word[0] == '<')
                {
                    int pos = iter->pos - 1;
                    char *lineText = (char *)(iter->line + pos);
                    int length = iter->lineLength - pos;
                    CbColorWord(cb, colors.string, lineText, length);
                    break;
                }

                else if (strchr("()[]{};,", c) != NULL)
                    col = colors.bracket;
                else if (strchr("+-/*=~%<>&|?!", c) != NULL)
                    col = colors.symbol;
            }
        }

        CbColorWord(cb, col, tok.word, tok.wordLength);
    }
}