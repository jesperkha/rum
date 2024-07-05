#include "rum.h"

extern Config config;
extern Editor editor;

static inline bool isSeperator(char c)
{
    return !((c >= 'a' && c <= 'z') ||
             (c >= 'A' && c <= 'Z') ||
             (c >= '0' && c <= '9') ||
             c == '_');
}

int FindNextWordBegin()
{
    bool startOnWord = !isSeperator(curChar);
    bool hitSpace = false;

    for (int i = curCol; i < curLine.length; i++)
    {
        char c = curLine.chars[i];
        if (c == ' ')
        {
            hitSpace = true;
            continue;
        }

        if (hitSpace)
        {
            startOnWord = isSeperator(c);
            hitSpace = false;
        }

        if (startOnWord && isSeperator(c))
            return i;
        if (!startOnWord && !isSeperator(c))
            return i;
    }

    return curLine.length - 1;
}

int FindPrevWordBegin()
{
    // It works, do not touch
    if (curCol == 0)
        return 0;

    char first = curChar;
    int start = curCol;
    bool hitSpace = false;

    for (int i = curCol - 1; i > 0; i--)
    {
        char c = curLine.chars[i];
        if (!hitSpace && (isSeperator(c) != isSeperator(first) || c == ' '))
        {
            if (i + 1 == start)
                first = c;
            else
                return i + 1;
        }

        if (c == ' ')
        {
            hitSpace = true;
            continue;
        }

        if (hitSpace)
        {
            hitSpace = false;
            first = c;
        }
    }

    return 0;
}

int FindLineBegin()
{
    return curLine.indent;
}

int FindLineEnd()
{
    return curLine.length - 1;
}

int FindNextChar(char c, bool backwards)
{
    int start = curCol;
    if (backwards)
    {
        for (int i = start - 1; i > 0; i--)
            if (curLine.chars[i] == c)
                return i;
    }
    else
    {
        for (int i = start + 1; i < curLine.length; i++)
            if (curLine.chars[i] == c)
                return i;
    }
    return start;
}

CursorPos FindNext(char *search, int length)
{
    // Todo: FindNext
    return (CursorPos){};
}

static bool isBlank(Line line)
{
    return line.indent == line.length;
}

int FindNextBlankLine()
{
    bool startedOnBlank = isBlank(curLine);

    for (int i = curRow; i < curBuffer->numLines; i++)
    {
        if (startedOnBlank)
        {
            startedOnBlank = isBlank(curBuffer->lines[i]);
            continue;
        }
        if (isBlank(curBuffer->lines[i]))
            return i;
    }

    return curBuffer->numLines - 1;
}

int FindPrevBlankLine()
{
    bool startedOnBlank = isBlank(curLine);

    for (int i = curRow; i > 0; i--)
    {
        if (startedOnBlank)
        {
            startedOnBlank = isBlank(curBuffer->lines[i]);
            continue;
        }
        if (isBlank(curBuffer->lines[i]))
            return i;
    }

    return 0;
}