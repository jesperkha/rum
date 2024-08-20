#include "rum.h"

extern Editor editor;
extern Colors colors;

// Buffer written to and rendered with highlights.
// Note: may cause segfault with long ass lines.
#define HL_BUFSIZE 2048
char hlBuffer[HL_BUFSIZE];

#define IS_NUMBER(n) (n >= '0' && n <= '9')
#define fg(buf, col) CbFg(buf, col)

// Returns pointer to character after the seperator found. Returns NULL on not found.
static char *findSeperator(char *line)
{
    while (*line != 0)
    {
        if (strchr("\"',.()+-/*=~%[];:{}<>&|?!# ", *line) != NULL)
            return line + 1;
        line++;
    }
    return NULL;
}

// Matches word in line to keyword list and adds highlight.
static void addKeyword(Buffer *b, CharBuf *cb, char *src, int length)
{
    if (length <= 0)
        return;

    // Get null terminated array with word
    char word[length + 1];
    memcpy(word, src, length);
    word[length] = 0;

    // Check if number first - pink
    if (IS_NUMBER(word[0]))
    {
        fg(cb, colors.number);
        CbAppend(cb, src, length);
        fg(cb, colors.fg0);
        return;
    }

    // Keep track if a highlight was added. To minimize line length,
    // the color only resets after colored words not for each word or symbol.
    bool colored = false;

    // Check if word is keyword or type name from loaded syntax set
    char *cols[2] = {colors.keyword, colors.type};

    for (int i = 0; i < 2; i++)
    {
        char *kw = b->syntaxTable->words[i];
        for (int j = 0; j < b->syntaxTable->numWords[i]; j++)
        {
            if (!strcmp(kw, word))
            {
                fg(cb, cols[i]);
                colored = true;
                break;
            }

            kw = memchr(kw, 0, 1024) + 1;
        }

        if (colored)
            break;
    }

    // Add word to buffer
    CbAppend(cb, src, length);

    if (colored)
        fg(cb, colors.fg0);
}

// Matches the last seperator with symbol list and adds highlight.
static void addSymbol(CharBuf *cb, char *src)
{
    // Symbols are part of the seperator group and
    // findSeperator() returns pos+1
    char symbol = *(src - 1);
    bool colored = true;

    if (strchr("+-/*=~%<>&|?!", symbol) != NULL)
        // Match operand symbol - aqua
        fg(cb, colors.symbol);
    else if (strchr("(){}[];,", symbol) != NULL)
        // Match notation symbol - grey
        fg(cb, colors.bracket);
    else
        colored = false;

    // Add symbol to buffer
    CbAppend(cb, src - 1, 1);

    if (colored)
        fg(cb, colors.fg0);
}

void addTextHighlight(Buffer *b, CharBuf *cb);

// Returns pointer to highlight buffer. Must NOT be freed. Line is the
// pointer to the line contents and the length is excluding the NULL
// terminator. Writes byte length of highlighted text to newLength.
char *HighlightLine(Buffer *b, char *line, int lineLength, int *newLength)
{
    // int fileType = editor.info.fileType;
    // Todo: comment file types in highlight
    int fileType = FT_C; // Debug
    *newLength = lineLength;

    // Todo: highlight gets messed up when scrolling horizontally

    if (!strcmp(b->syntaxTable->extension, "py"))
        fileType = FT_PYTHON;

    if (lineLength == 0)
        return line;

    // Keep track of last pos and the seperator stopped at
    char *sep = line;
    char *prev = line;
    CharBuf cb = CbNew(hlBuffer);

    while ((sep = findSeperator(sep)) != NULL)
    {
        // Seperator out of bounds
        if (sep - line > lineLength)
            break;

        if (cb.lineLength >= HL_BUFSIZE) // Debug
        {
            Errorf("Highlight buffer overflow %d", cb.lineLength);
            *newLength = lineLength;
            return line;
        }

        // Get word length and add highlight for word and symbol
        int length = sep - prev - 1;
        char symbol = *(sep - 1);

        if (symbol == '(')
        {
            // Function call/name - yellow
            fg(&cb, colors.function);
            CbAppend(&cb, prev, length);
        }
        else if (*prev == '#' && fileType == FT_C)
        {
            // Macro definition - aqua
            fg(&cb, colors.symbol);
            CbAppend(&cb, prev, length);
        }
        else if (symbol == '.')
        {
            if (IS_NUMBER(*prev)) // Float - pink
                fg(&cb, colors.number);
            else // Object - blue
                fg(&cb, colors.object);

            CbAppend(&cb, prev, length);
        }
        else if (length > 0)
            // Normal keyword
            addKeyword(b, &cb, prev, length);

        if (strchr("'\"<", symbol) != NULL)
        {
            // Only highlight macro strings
            if (symbol == '<' && line[0] != '#')
                goto add_symbol;

            // Strings - green
            fg(&cb, colors.string);

            // Get next quote
            char endSym = symbol == '<' ? '>' : symbol;
            char *end = strchr(sep, endSym);
            if (end == NULL || end >= line + lineLength)
            {
                // If unterminated just add rest of line
                CbAppend(&cb, sep - 1, (line + lineLength) - sep + 1);
                *newLength = cb.pos - cb.buffer;
                return cb.buffer;
            }

            // Add string contents to buffer
            CbAppend(&cb, sep - 1, end - sep + 2);
            sep = end + 1;
            prev = sep;
            fg(&cb, colors.fg0);
            continue; // Skip addSymbol
        }
        else if (
            (fileType == FT_C && symbol == '/' && *(sep) == '/') ||
            (fileType == FT_PYTHON && symbol == '#'))
        {
            // int cmtLen = strlen(b->syntaxTable->comment);
            // bool matched = true;
            // for (int i = 0; i < cmtLen && sep + i < line + lineLength; i++)
            // {
            //     if (*(sep + i - 1) != b->syntaxTable->comment[i])
            //         matched = false;
            // }

            // Comment - grey
            fg(&cb, colors.bg2);
            CbAppend(&cb, sep - 1, (line + lineLength) - sep + 1);
            *newLength = cb.pos - cb.buffer;
            return cb.buffer;
        }

    add_symbol:

        // Normal symbol
        addSymbol(&cb, sep);
        prev = sep;
    }

    // Remaining after last seperator
    addKeyword(b, &cb, prev, (line + lineLength) - prev);
    *newLength = cb.pos - cb.buffer;
    addTextHighlight(b, &cb);
    return cb.buffer;
}

// Adds selection highlighting (white background) to text
void addTextHighlight(Buffer *b, CharBuf *cb)
{
    // Todo: (feature) text highlighting
    // need to give Line object to highlight function to know which row we are at
}