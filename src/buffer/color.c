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
static void addKeyword(Buffer *b, CharBuf *buf, char *src, int length)
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
        fg(buf, colors.number);
        CbAppend(buf, src, length);
        fg(buf, colors.fg0);
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
                fg(buf, cols[i]);
                colored = true;
                break;
            }

            kw = memchr(kw, 0, 1024) + 1;
        }

        if (colored)
            break;
    }

    // Add word to buffer
    CbAppend(buf, src, length);

    if (colored)
        fg(buf, colors.fg0);
}

// Matches the last seperator with symbol list and adds highlight.
static void addSymbol(CharBuf *buf, char *src)
{
    // Symbols are part of the seperator group and
    // findSeperator() returns pos+1
    char symbol = *(src - 1);
    bool colored = true;

    if (strchr("+-/*=~%<>&|?!", symbol) != NULL)
        // Match operand symbol - aqua
        fg(buf, colors.symbol);
    else if (strchr("(){}[];,", symbol) != NULL)
        // Match notation symbol - grey
        fg(buf, colors.bracket);
    else
        colored = false;

    // Add symbol to buffer
    CbAppend(buf, src - 1, 1);

    if (colored)
        fg(buf, colors.fg0);
}

// Todo: text highlighting

// Returns pointer to highlight buffer. Must NOT be freed. Line is the
// pointer to the line contents and the length is excluding the NULL
// terminator. Writes byte length of highlighted text to newLength.
char *HighlightLine(Buffer *b, char *line, int lineLength, int *newLength)
{
    // int fileType = editor.info.fileType;
    // Todo: comment file types in highlight
    int fileType = FT_C; // Debug
    *newLength = lineLength;

    if (lineLength == 0)
        return line;

    // Keep track of last pos and the seperator stopped at
    char *sep = line;
    char *prev = line;

    // Only used to keep track of line length
    CharBuf buffer = {
        .buffer = hlBuffer,
        .pos = hlBuffer,
        .lineLength = 0,
    };

    while ((sep = findSeperator(sep)) != NULL)
    {
        // Seperator out of bounds
        if (sep - line > lineLength)
            break;

        if (buffer.lineLength >= HL_BUFSIZE) // Debug
        {
            Errorf("Highlight buffer overflow %d", buffer.lineLength);
            *newLength = lineLength;
            return line;
        }

        // Get word length and add highlight for word and symbol
        int length = sep - prev - 1;
        char symbol = *(sep - 1);

        if (symbol == '(')
        {
            // Function call/name - yellow
            fg(&buffer, colors.function);
            CbAppend(&buffer, prev, length);
        }
        else if (*prev == '#' && fileType == FT_C)
        {
            // Macro definition - aqua
            fg(&buffer, colors.symbol);
            CbAppend(&buffer, prev, length);
        }
        else if (symbol == '.')
        {
            if (IS_NUMBER(*prev)) // Float - pink
                fg(&buffer, colors.number);
            else // Object - blue
                fg(&buffer, colors.object);

            CbAppend(&buffer, prev, length);
        }
        else if (length > 0)
            // Normal keyword
            addKeyword(b, &buffer, prev, length);

        if (strchr("'\"<", symbol) != NULL)
        {
            // Only highlight macro strings
            if (symbol == '<' && line[0] != '#')
                goto add_symbol;

            // Strings - green
            fg(&buffer, colors.string);

            // Get next quote
            char endSym = symbol == '<' ? '>' : symbol;
            char *end = strchr(sep, endSym);
            if (end == NULL || end >= line + lineLength)
            {
                // If unterminated just add rest of line
                CbAppend(&buffer, sep - 1, (line + lineLength) - sep + 1);
                *newLength = buffer.pos - buffer.buffer;
                return buffer.buffer;
            }

            // Add string contents to buffer
            CbAppend(&buffer, sep - 1, end - sep + 2);
            sep = end + 1;
            prev = sep;
            fg(&buffer, colors.fg0);
            continue; // Skip addSymbol
        }
        else if (
            (fileType == FT_C && symbol == '/' && *(sep) == '/') ||
            (fileType == FT_PYTHON && symbol == '#'))
        {
            // Comment - grey
            fg(&buffer, colors.bg2);
            CbAppend(&buffer, sep - 1, (line + lineLength) - sep + 1);
            *newLength = buffer.pos - buffer.buffer;
            return buffer.buffer;
        }

    add_symbol:

        // Normal symbol
        addSymbol(&buffer, sep);
        prev = sep;
    }

    // Remaining after last seperator
    addKeyword(b, &buffer, prev, (line + lineLength) - prev);
    *newLength = buffer.pos - buffer.buffer;
    return buffer.buffer;
}