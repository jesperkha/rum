#include "wim.h"
#include "util.h"

// Buffer written to and rendered with highlights
#define LEXER_BUFSIZE 2048
char lexerBuffer[LEXER_BUFSIZE];

char *keywords[] = {
    "auto", "break", "case", "continue", "default", "do", "else", "enum",
    "extern", "for", "goto", "if", "register", "return", "sizeof", "static",
    "struct", "switch", "typedef", "union", "volatile", "while", "NULL"};

char *types[] = {
    "int", "long", "double", "float", "char", "unsigned", "signed",
    "void", "short", "auto", "const", "bool"};

#define NUM_KEYWORDS 23
#define NUM_TYPES 12

#define IS_NUMBER(n) (n >= '0' && n <= '9')

// Returns pointer to character after the seperator found.
// Returns NULL on not found.
static char *findSeperator(char *line)
{
    while (*line != 0)
    {
        if (strchr(",.()+-/*=~%[];{}<>&|?! ", *line) != NULL)
            return line + 1;
        line++;
    }

    return NULL;
}

// Matches word in line to keyword list and adds highlight.
static void addKeyword(CharBuffer *buf, char *src, int length)
{
    if (length == 0)
        return;

    // Get null terminated array with word
    char word[length + 1];
    memcpy(word, src, length);
    word[length] = 0;

    // Check if number first - pink
    if (IS_NUMBER(word[0]))
    {
        charbufAppend(buf, FG(COL_PINK), strlen(FG(COL_PINK)));
        charbufAppend(buf, src, length);
        charbufAppend(buf, FG(COL_FG0), strlen(FG(COL_FG0)));
        return;
    }

    // Keep track if a highlight was added. To minimize line length,
    // the color only resets after colored words not for each word or symbol.
    bool colored = false;

    for (int i = 0; i < max(NUM_KEYWORDS, NUM_TYPES); i++)
    {
        // Keyword highlight - red
        if (i < NUM_KEYWORDS && !strcmp(keywords[i], word))
        {
            charbufAppend(buf, FG(COL_RED), strlen(FG(COL_RED)));
            colored = true;
            break;
        }

        // Type name highlight - orange
        if (i < NUM_TYPES && !strcmp(types[i], word))
        {
            charbufAppend(buf, FG(COL_ORANGE), strlen(FG(COL_ORANGE)));
            colored = true;
            break;
        }
    }

    // Add word to buffer
    charbufAppend(buf, src, length);

    if (colored)
        charbufAppend(buf, FG(COL_FG0), strlen(FG(COL_FG0)));
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
        charbufAppend(buf, FG(COL_AQUA), strlen(FG(COL_AQUA)));
    else if (strchr("(){}[];,", symbol) != NULL)
        // Match notation symbol - grey
        charbufAppend(buf, FG(COL_GREY), strlen(FG(COL_GREY)));
    else
        colored = false;

    // Add symbol to buffer
    charbufAppend(buf, src - 1, 1);

    if (colored)
        charbufAppend(buf, FG(COL_FG0), strlen(FG(COL_FG0)));
}

// Returns pointer to highlight buffer. Must NOT be freed. Line is the
// pointer to the line contents and the length is excluding the NULL
// terminator. Writes byte length of highlighted text to newLength.
char *highlightLine(char *line, int lineLength, int *newLength)
{
    *newLength = lineLength;

    if (lineLength == 0)
        return line;

    // Keep track of last pos and the seperator stopped at
    char *sep = line;
    char *prev = line;

    // Only used to keep track of line length
    CharBuffer buffer = {
        .buffer = lexerBuffer,
        .pos = lexerBuffer,
        .lineLength = 0,
    };

    while ((sep = findSeperator(sep)) != NULL)
    {
        // Seperator out of bounds
        if (sep - line > lineLength)
            break;

        if (buffer.lineLength >= LEXER_BUFSIZE) // Debug
        {
            logNumber("Lexer buffer overflow", buffer.lineLength);
            *newLength = lineLength;
            return line;
        }

        // Todo: string syntax highlighting

        // if (*prev == '"')
        // {
        //     charbufAppend(&buffer, FG(COL_RED), strlen(FG(COL_RED)));

        //     char *end = strchr(prev + 1, '"');
        //     int length;

        //     if (end != NULL)
        //     {
        //         length = end - prev;
        //         charbufAppend(&buffer, prev, length);
        //         prev = end;
        //         sep = prev;
        //     }
        //     else
        //     {
        //         charbufAppend(&buffer, prev, (line + lineLength) - prev);
        //         *newLength = buffer.lineLength;
        //         return buffer.buffer;
        //     }

        //     charbufAppend(&buffer, FG(COL_FG0), strlen(FG(COL_FG0)));
        //     continue;
        // }

        // Comment highlight - dark grey
        if (*prev == '/' && *(prev + 1) == '/')
        {
            charbufAppend(&buffer, FG(COL_BG3), strlen(FG(COL_BG3)));
            charbufAppend(&buffer, prev, (line + lineLength) - prev);
            *newLength = buffer.lineLength;
            return buffer.buffer;
        }

        // Get word length and add highlight for word and symbol
        int length = sep - prev - 1;
        char symbol = *(sep - 1);

        if (symbol == '(')
        {
            // Function call/name - yellow
            charbufAppend(&buffer, FG(COL_YELLOW), strlen(FG(COL_YELLOW)));
            charbufAppend(&buffer, prev, length);
        }
        else if (*prev == '#')
        {
            // Macro definition - aqua
            charbufAppend(&buffer, FG(COL_AQUA), strlen(FG(COL_AQUA)));
            charbufAppend(&buffer, prev, length);
        }
        else if (symbol == '.')
        {
            if (IS_NUMBER(*prev)) // Float - pink
                charbufAppend(&buffer, FG(COL_PINK), strlen(FG(COL_PINK)));
            else // Object - blue
                charbufAppend(&buffer, FG(COL_BLUE), strlen(FG(COL_BLUE)));

            charbufAppend(&buffer, prev, length);
        }
        else if (length > 0)
            addKeyword(&buffer, prev, length);

        addSymbol(&buffer, sep);
        prev = sep;
    }

    // Remaining after last seperator
    addKeyword(&buffer, prev, (line + lineLength) - prev);
    *newLength = buffer.lineLength;
    return buffer.buffer;
}