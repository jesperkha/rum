#include "editor.h"
#include "util.h"
#include "lexer.h"

char lexerBuffer[1024];

struct strbuf
{
    char *buffer;
    int length;
};

static void strbufAppend(struct strbuf *buf, char *src, int length)
{
    memcpy(buf->buffer + buf->length, src, length);
    buf->length += length;
}

static char *findSeperator(char *line)
{
    while (*line != 0)
    {
        if (strchr(",.()+-/*=~%[];{}<> ", *line) != NULL)
            return line + 1;

        line++;
    }

    return NULL;
}

static void addKeyword(struct strbuf *buf, char *src, int length)
{
    if (length == 0)
        return;

    char b[length + 1];
    memcpy(b, src, length);
    b[length] = 0;

    // Todo: compare keywords etc
    if (!strcmp("word", b))
        strbufAppend(buf, FG(COL_YELLOW), strlen(FG(COL_YELLOW)));
    else
        strbufAppend(buf, FG(COL_FG0), strlen(FG(COL_FG0)));

    strbufAppend(buf, src, length);
}

static void addSymbol(struct strbuf *buf, char *src)
{
    strbufAppend(buf, src - 1, 1);

    char c = *(src - 1);
    if (c == ' ')
        return;
}

char *highlightLine(char *line, int length, int *newLength)
{
    *newLength = length;

    if (length == 0)
        return line;

    char *sep = line;
    char *prev = line;

    struct strbuf buffer = {
        .buffer = lexerBuffer,
        .length = 0,
    };

    while ((sep = findSeperator(sep)) != NULL)
    {
        int length = sep - prev - 1;

        if (length != 0)
            addKeyword(&buffer, prev, length);

        addSymbol(&buffer, sep);
        prev = sep;
    }

    addKeyword(&buffer, prev, (line + length) - prev);
    *newLength = buffer.length;
    return buffer.buffer;
}