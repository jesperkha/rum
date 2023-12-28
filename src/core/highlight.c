#include "core.h"
#include "util.h"

// Buffer written to and rendered with highlights
#define HL_BUFSIZE 2048
char hlBuffer[HL_BUFSIZE];

// Todo: load from file

char *keywords_c[] = {
    "auto", "break", "case", "continue", "default", "do", "else", "enum",
    "extern", "for", "goto", "if", "register", "return", "sizeof", "static",
    "struct", "switch", "typedef", "union", "volatile", "while", "NULL",
    "true", "false"};

char *types_c[] = {
    "int", "long", "double", "float", "char", "unsigned", "signed",
    "void", "short", "auto", "const", "bool"};

char *keywords_py[] = {
    "False", "await", "else", "import",	"pass",
    "True",	"class", "finally",	"is", "return",
    "and", "continue", "for", "lambda",	"try",
    "as", "def", "from", "nonlocal", "while",
    "assert", "del", "global", "not", "with",
    "async", "elif", "if", "or", "yield",
    "break", "except", "in", "raise" };

char *types_py[] = {
    "int", "float", "str", "dict", "list",
    "None", "bool", "complex", "tuple", "range",
    "set", "set", "bytes"};

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))
#define IS_NUMBER(n) (n >= '0' && n <= '9')

#define color(buf, col) charbufAppend(buf, col, strlen(col))

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
    // Note: remove asap
    int fileType = editorGetHandle()->info.fileType;
    char **keywords = keywords_c;
    char **types = types_c;
    int numKeywords = ARRAY_LEN(keywords_c);
    int numTypes = ARRAY_LEN(types_c);
    if (fileType == FT_PYTHON)
    {
        keywords = keywords_py;
        types = types_py;
        numKeywords = ARRAY_LEN(keywords_py);
        numTypes = ARRAY_LEN(types_py);
    }

    if (length <= 0)
        return;

    // Get null terminated array with word
    char word[length + 1];
    memcpy(word, src, length);
    word[length] = 0;

    // Check if number first - pink
    if (IS_NUMBER(word[0]))
    {
        color(buf, FG(COL_PINK));
        charbufAppend(buf, src, length);
        color(buf, FG(COL_FG0));
        return;
    }

    // Keep track if a highlight was added. To minimize line length,
    // the color only resets after colored words not for each word or symbol.
    bool colored = false;

    for (int i = 0; i < max(numKeywords, numTypes); i++)
    {
        // Keyword highlight - red
        if (i < numKeywords && !strcmp(keywords[i], word))
        {
            color(buf, FG(COL_RED));
            colored = true;
            break;
        }

        // Type name highlight - orange
        if (i < numTypes && !strcmp(types[i], word))
        {
            color(buf, FG(COL_ORANGE));
            colored = true;
            break;
        }
    }

    // Add word to buffer
    charbufAppend(buf, src, length);

    if (colored)
        color(buf, FG(COL_FG0));
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
        color(buf, FG(COL_AQUA));
    else if (strchr("(){}[];,", symbol) != NULL)
        // Match notation symbol - grey
        color(buf, FG(COL_GREY));
    else
        colored = false;

    // Add symbol to buffer
    charbufAppend(buf, src - 1, 1);

    if (colored)
        color(buf, FG(COL_FG0));
}

// Returns pointer to highlight buffer. Must NOT be freed. Line is the
// pointer to the line contents and the length is excluding the NULL
// terminator. Writes byte length of highlighted text to newLength.
char *highlightLine(char *line, int lineLength, int *newLength)
{
    int fileType = editorGetHandle()->info.fileType;
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
            logNumber("Highlight buffer overflow", buffer.lineLength);
            *newLength = lineLength;
            return line;
        }

        // Get word length and add highlight for word and symbol
        int length = sep - prev - 1;
        char symbol = *(sep - 1);

        if (symbol == '(')
        {
            // Function call/name - yellow
            color(&buffer, FG(COL_YELLOW));
            charbufAppend(&buffer, prev, length);
        }
        else if (*prev == '#' && fileType == FT_C)
        {
            // Macro definition - aqua
            color(&buffer, FG(COL_AQUA));
            charbufAppend(&buffer, prev, length);
        }
        else if (symbol == '.')
        {
            if (IS_NUMBER(*prev)) // Float - pink
                color(&buffer, FG(COL_PINK));
            else // Object - blue
                color(&buffer, FG(COL_BLUE));

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
            color(&buffer, FG(COL_GREEN));

            // Get next quote
            char endSym = symbol == '<' ? '>' : symbol;
            char *end = strchr(sep, endSym);
            if (end == NULL || end >= line + lineLength)
            {
                // If unterminated just add rest of line
                charbufAppend(&buffer, sep - 1, (line + lineLength) - sep + 1);
                *newLength = buffer.lineLength;
                return buffer.buffer;
            }

            // Add string contents to buffer
            charbufAppend(&buffer, sep - 1, end - sep + 2);
            sep = end+1;
            prev = sep;
            color(&buffer, FG(COL_FG0));
            continue; // Skip addSymbol
        }
        else if (
            (fileType == FT_C && symbol == '/' && *(sep) == '/') ||
            (fileType == FT_PYTHON && symbol == '#'))
        {
            // Comment - grey
            color(&buffer, FG(COL_BG2));
            charbufAppend(&buffer, sep - 1, (line + lineLength) - sep + 1);
            *newLength = buffer.lineLength;
            return buffer.buffer;
        }

    add_symbol:

        // Normal symbol
        addSymbol(&buffer, sep);
        prev = sep;
    }

    // Remaining after last seperator
    addKeyword(&buffer, prev, (line + lineLength) - prev);
    *newLength = buffer.lineLength;
    return buffer.buffer;
}