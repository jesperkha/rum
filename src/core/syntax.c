#include "wim.h"

extern Editor editor;

// Buffer written to and rendered with highlights
#define HL_BUFSIZE 2048
char hlBuffer[HL_BUFSIZE];

#define IS_NUMBER(n) (n >= '0' && n <= '9')
#define fg(buf, col) charbufFg(buf, col)

// Returns pointer to character after the seperator found.
// Returns NULL on not found.
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
static void addKeyword(CharBuffer *buf, char *src, int length)
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
        fg(buf, COL_PINK);
        charbufAppend(buf, src, length);
        fg(buf, COL_FG0);
        return;
    }

    // Keep track if a highlight was added. To minimize line length,
    // the color only resets after colored words not for each word or symbol.
    bool colored = false;

    // Check if word is keyword or type name from loaded syntax set
    const int colors[2] = {COL_RED, COL_ORANGE};

    for (int i = 0; i < 2; i++)
    {
        char *kw = editor.syntaxTable.syn[i];
        for (int j = 0; j < editor.syntaxTable.len[i]; j++)
        {
            if (!strcmp(kw, word))
            {
                fg(buf, colors[i]);
                colored = true;
                break;   
            }

            kw = memchr(kw, 0, 1024)+1;
        }

        if (colored)
            break;
    }

    // Add word to buffer
    charbufAppend(buf, src, length);

    if (colored)
        fg(buf, COL_FG0);
}

// Matches the last seperator with symbol list and adds highlight.
static void addSymbol(CharBuffer *buf, char *src)
{
    // Symbols are part of the seperator group and
    // findSeperator() returns pos+1
    char symbol = *(src - 1);
    bool colored = true;

    if (strchr("+-/*=~%<>&|?!", symbol) != NULL)
        // Match operand symbol - aqua
        fg(buf, COL_AQUA);
    else if (strchr("(){}[];,", symbol) != NULL)
        // Match notation symbol - grey
        fg(buf, COL_GREY);
    else
        colored = false;

    // Add symbol to buffer
    charbufAppend(buf, src - 1, 1);

    if (colored)
        fg(buf, COL_FG0);
}

// Returns pointer to highlight buffer. Must NOT be freed. Line is the
// pointer to the line contents and the length is excluding the NULL
// terminator. Writes byte length of highlighted text to newLength.
char *highlightLine(char *line, int lineLength, int *newLength)
{
    int fileType = editor.info.fileType;
    *newLength = lineLength;

    if (lineLength == 0)
        return line;

    // Keep track of last pos and the seperator stopped at
    char *sep = line;
    char *prev = line;

    // Only used to keep track of line length
    CharBuffer buffer = {
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
            LogNumber("Highlight buffer overflow", buffer.lineLength);
            *newLength = lineLength;
            return line;
        }

        // Get word length and add highlight for word and symbol
        int length = sep - prev - 1;
        char symbol = *(sep - 1);

        if (symbol == '(')
        {
            // Function call/name - yellow
            fg(&buffer, COL_YELLOW);
            charbufAppend(&buffer, prev, length);
        }
        else if (*prev == '#' && fileType == FT_C)
        {
            // Macro definition - aqua
            fg(&buffer, COL_AQUA);
            charbufAppend(&buffer, prev, length);
        }
        else if (symbol == '.')
        {
            if (IS_NUMBER(*prev)) // Float - pink
                fg(&buffer, COL_PINK);
            else // Object - blue
                fg(&buffer, COL_BLUE);

            charbufAppend(&buffer, prev, length);
        }
        else if (length > 0)
            // Normal keyword
            addKeyword(&buffer, prev, length);

        if (strchr("'\"<", symbol) != NULL)
        {
            // Only highlight macro strings
            if (symbol == '<' && line[0] != '#')
                goto add_symbol;

            // Strings - green
            fg(&buffer, COL_GREEN);

            // Get next quote
            char endSym = symbol == '<' ? '>' : symbol;
            char *end = strchr(sep, endSym);
            if (end == NULL || end >= line + lineLength)
            {
                // If unterminated just add rest of line
                charbufAppend(&buffer, sep - 1, (line + lineLength) - sep + 1);
                *newLength = buffer.pos - buffer.buffer;
                return buffer.buffer;
            }

            // Add string contents to buffer
            charbufAppend(&buffer, sep - 1, end - sep + 2);
            sep = end+1;
            prev = sep;
            fg(&buffer, COL_FG0);
            continue; // Skip addSymbol
        }
        else if (
            (fileType == FT_C && symbol == '/' && *(sep) == '/') ||
            (fileType == FT_PYTHON && symbol == '#'))
        {
            // Comment - grey
            fg(&buffer, COL_BG2);
            charbufAppend(&buffer, sep - 1, (line + lineLength) - sep + 1);
            *newLength = buffer.pos - buffer.buffer;
            return buffer.buffer;
        }

    add_symbol:

        // Normal symbol
        addSymbol(&buffer, sep);
        prev = sep;
    }

    // Remaining after last seperator
    addKeyword(&buffer, prev, (line + lineLength) - prev);
    *newLength = buffer.pos - buffer.buffer;
    return buffer.buffer;
}