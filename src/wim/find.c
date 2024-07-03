#include "wim.h"

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
    bool startOnWord = !isSeperator(curLine.chars[curCol]);
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
