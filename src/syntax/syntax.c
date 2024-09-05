#include "rum.h"

extern Editor editor;
extern Colors colors;

// Buffer written to and rendered with highlights.
// Note: may cause segfault with long ass lines.
#define HL_BUFSIZE 2048
char hlBuffer[HL_BUFSIZE];

// Returns true if sequence was found, and also keeps iteration made to iter.
// Otherwise iteration is reset to where it was.
static bool matchSymbolSequence(LineIterator *iter, char *sequence)
{
    int prevPos = iter->pos; // Reset to this if comment not found
    bool matched = true;

    for (int i = 1; i < strlen(sequence); i++)
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

// Adds highlight to marked areas and returns new line pointer.
static HlLine highlightLine(char *coloredLine, int length)
{
    return (HlLine){
        .line = coloredLine,
        .length = length,
    };
}

// Returns pointer to highlight buffer. Must NOT be freed. Line is the
// pointer to the line contents and the length is excluding the NULL
// terminator. Writes byte length of highlighted text to newLength.
HlLine ColorLine(Buffer *b, char *line, int lineLength)
{
    if (lineLength == 0)
        return (HlLine){.line = line, .length = lineLength};

    CharBuf cb = CbNew(hlBuffer);
    LineIterator iter = NewLineIterator(line, lineLength);

    // Todo: block comments
    char *comment = "//";

    while (true)
    {
        SyntaxToken tok = GetNextToken(&iter);
        if (tok.eof)
            break;

        char *col = colors.fg0;
        bool colored = false;

        if (tok.isString)
            col = colors.string;
        else if (tok.isNumber)
            col = colors.number;
        else if (tok.isWord)
        {
            // Function name
            {
                int prevPos = iter.pos;
                SyntaxToken next = GetNextToken(&iter);
                if (next.isSymbol && next.word[0] == '(')
                {
                    col = colors.function;
                    colored = true;
                }
                iter.pos = prevPos;
            }

            // Reserved keyword or type name
            {
                char *cols[2] = {colors.keyword, colors.type};

                for (int i = 0; i < 2; i++)
                {
                    char *kw = b->syntaxTable->words[i];
                    for (int j = 0; j < b->syntaxTable->numWords[i]; j++)
                    {
                        if (!strcmp(kw, tok.word))
                        {
                            col = cols[i];
                            colored = true;
                            break;
                        }

                        kw = memchr(kw, 0, 1024) + 1;
                    }

                    if (colored)
                        break;
                }
            }

            // C user types (just checks first letter is capitalized)
            if (!colored && isupper(tok.word[0]))
            {
                col = colors.userType;
                colored = true;
            }
        }
        else if (tok.isSymbol)
        {
            char c = tok.word[0];

            // .foo
            if (c == '.')
            {
                int prevPos = iter.pos;
                SyntaxToken next = GetNextToken(&iter);
                if (next.isWord)
                {
                    CbColorWord(&cb, colors.bracket, tok.word, tok.wordLength);
                    CbColorWord(&cb, colors.object, next.word, next.wordLength);
                    continue;
                }
                iter.pos = prevPos;
            }

            // ->foo
            if (c == '-')
            {
                int prevPos = iter.pos;
                SyntaxToken next = GetNextToken(&iter);
                SyntaxToken word = GetNextToken(&iter);
                if (next.isSymbol && next.word[0] == '>' && word.isWord)
                {
                    CbColorWord(&cb, colors.bracket, "->", 2);
                    CbColorWord(&cb, colors.object, word.word, word.wordLength);
                    continue;
                }
                iter.pos = prevPos;
            }

            // Single line comment
            if (c == comment[0])
            {
                int commentStart = iter.pos - 1;
                if (matchSymbolSequence(&iter, comment))
                {
                    CbFg(&cb, colors.bg2);
                    CbAppend(&cb, (char *)(iter.line + commentStart), iter.lineLength - commentStart);
                    break;
                }
            }

            // C macros
            if (c == '#')
            {
                CbColorWord(&cb, colors.bracket, tok.word, tok.wordLength);
                tok = GetNextToken(&iter); // Macro type name as well
                CbColorWord(&cb, colors.symbol, tok.word, tok.wordLength);
                continue;
            }

            // Normal symbols
            if (!colored)
            {
                if (strchr("()[]{};,", c) != NULL)
                    col = colors.bracket;
                else if (strchr("+-/*=~%<>&|?!", c) != NULL)
                    col = colors.symbol;
            }
        }

        CbColorWord(&cb, col, tok.word, tok.wordLength);
    }

    return highlightLine(cb.buffer, CbLength(&cb));
}
