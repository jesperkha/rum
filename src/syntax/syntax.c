#include "rum.h"

extern Editor editor;
extern Colors colors;
extern Config config;

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

// Inserts highlight color at column a to b. b can be -1 to indicate end of line.
static void highlightFromTo(HlLine *line, int a, int b, char *color)
{
    if (a == b)
        return;

    // Switch to using hlbuffer
    strncpy(hlBuffer, line->line, line->length);
    line->line = hlBuffer;

    int rawLength = 0;
    int colLen = COLOR_BYTE_LENGTH;

    char hlColor[COLOR_BYTE_LENGTH + 8];
    sprintf(hlColor, "\x1b[48;2;%sm", color);

    char nonHlColor[COLOR_BYTE_LENGTH + 8];
    char *lineBg = line->isCurrentLine ? colors.bg1 : colors.bg0;
    sprintf(nonHlColor, "\x1b[48;2;%sm", lineBg);

    for (int i = 0; i < line->length; i++)
    {
        char c = line->line[i];
        if (c == '\x1b')
        {
            i += colLen - 1;
            continue;
        }

        if (rawLength == a || (b != -1 && rawLength == b))
        {
            memmove(line->line + i + colLen, line->line + i, line->length - i);
            memcpy(line->line + i, rawLength == a ? hlColor : nonHlColor, colLen);
            line->length += colLen;
            i += colLen;
        }

        rawLength++;
    }

    if (b == -1)
        memcpy(line->line + line->length, COL_RESET, 4);

    memcpy(line->line + line->length, nonHlColor, colLen);
    line->length += colLen;

    if (line->rawLength != rawLength)
        Panicf("line->rawLength=%d, rawLength=%d, length=%d, row=%d", line->rawLength, rawLength, line->length, line->row);

    AssertEqual(line->rawLength, rawLength);
}

HlLine MarkLine(HlLine line, int start, int end)
{
    if (editor.mode != MODE_VISUAL && editor.mode != MODE_VISUAL_LINE)
        highlightFromTo(&line, start, end, COL_HL);
    return line;
}

// Adds highlight to marked areas and returns new line pointer.
HlLine HighlightLine(Buffer *b, HlLine line)
{
    if (config.rawMode)
        return line;

    CursorPos start, end;
    BufferOrderHighlightPoints(b, &start, &end);

    if (line.row < start.row || line.row > end.row)
        return line;

    int from = 0;
    int to = -1;

    if (start.row == line.row)
        from = start.col;
    if (end.row == line.row)
        to = end.col;

    highlightFromTo(&line, from, to, colors.bg1);
    return line;
}

// Returns pointer to highlight buffer. Must NOT be freed. Line is the
// pointer to the line contents and the length is excluding the NULL
// terminator. Writes byte length of highlighted text to newLength.
HlLine ColorLine(Buffer *b, HlLine line)
{
    if (line.length == 0)
        return line;

    CharBuf cb = CbNew(hlBuffer);
    LineIterator iter = NewLineIterator(line.line, line.length);

    char *comment = "//";
    char *blockCommentStart = "/*";
    char *blockCommentEnd = "*/";
    static int blockCommentDepth = 0;

    bool isIncludeMacro = false;

    if (line.row == 0) // Reset for new render
        blockCommentDepth = 0;

    while (true)
    {
        SyntaxToken tok = GetNextToken(&iter);
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

                if (c == blockCommentStart[0] && matchSymbolSequence(&iter, blockCommentStart))
                {
                    CbColorWord(&cb, colors.bg2, blockCommentStart, strlen(blockCommentStart));
                    blockCommentDepth++;
                    continue;
                }

                if (c == blockCommentEnd[0] && matchSymbolSequence(&iter, blockCommentEnd))
                {
                    CbColorWord(&cb, colors.bg2, blockCommentEnd, strlen(blockCommentEnd));
                    blockCommentDepth--;
                    continue;
                }
            }
            CbColorWord(&cb, colors.bg2, tok.word, tok.wordLength);
            continue;
        }

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

                        kw = (char *)memchr(kw, 0, 1024) + 1;
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
                    CbColorWord(&cb, colors.bracket, ".", 1);
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
                int startPos = iter.pos - 1;
                if (matchSymbolSequence(&iter, comment))
                {
                    char *lineText = (char *)(iter.line + startPos);
                    int length = iter.lineLength - startPos;
                    CbColorWord(&cb, colors.bg2, lineText, length);
                    break;
                }
            }

            // Block comment begin
            if (c == blockCommentStart[0] && matchSymbolSequence(&iter, blockCommentStart))
            {
                CbColorWord(&cb, colors.bg2, blockCommentStart, strlen(blockCommentStart));
                blockCommentDepth++;
                continue;
            }

            // C macros
            if (c == '#')
            {
                CbColorWord(&cb, colors.bracket, tok.word, tok.wordLength);
                tok = GetNextToken(&iter); // Macro type name as well
                CbColorWord(&cb, colors.symbol, tok.word, tok.wordLength);
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
                    int pos = iter.pos - 1;
                    char *lineText = (char *)(iter.line + pos);
                    int length = iter.lineLength - pos;
                    CbColorWord(&cb, colors.string, lineText, length);
                    break;
                }

                else if (strchr("()[]{};,", c) != NULL)
                    col = colors.bracket;
                else if (strchr("+-/*=~%<>&|?!", c) != NULL)
                    col = colors.symbol;
            }
        }

        CbColorWord(&cb, col, tok.word, tok.wordLength);
    }

    if (line.isCurrentLine && !b->showHighlight && b->showCurrentLineMark)
        CbColor(&cb, colors.bg1, colors.fg0);
    else
        CbColor(&cb, colors.bg0, colors.fg0);

    HlLine hline = {
        .length = CbLength(&cb),
        .line = cb.buffer,
        .rawLength = line.length,
        .row = line.row,
        .isCurrentLine = line.isCurrentLine,
    };

    return hline;
}
