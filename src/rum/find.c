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

#define isBlank(l) ((l).indent == (l).length)

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

// Returns true if the search was found. Puts position to pos.
// Dir is 1 for downwards- and -1 for upwards search.
static bool find(char *search, int length, int dir, int startRow, CursorPos *pos)
{
    char firstc = search[0];

    for (int row = startRow;
         dir == 1 ? (row < curBuffer->numLines) : (row > 0);
         dir == 1 ? row++ : row--)
    {
        Line line = curBuffer->lines[row];
        for (int col = 0; col < line.length; col++)
        {
            char c = line.chars[col];
            if (c != firstc || col > line.length - length)
                continue;

            bool found = true;
            for (int i = 0; i < length; i++)
            {
                if (line.chars[col + i] != search[i])
                    found = false;
            }

            if (found)
            {
                pos->row = row;
                pos->col = col;
                return true;
            }
        }
    }

    return false;
}

CursorPos FindNext(char *search, int length)
{
    CursorPos pos = curPos;
    if (!find(search, length, 1, curRow + 1, &pos))
        find(search, length, 1, 0, &pos);
    return pos;
}

CursorPos FindPrev(char *search, int length)
{
    CursorPos pos = curPos;
    if (!find(search, length, -1, curRow - 1, &pos))
        find(search, length, -1, curBuffer->numLines - 1, &pos);
    return pos;
}

void FindPrompt()
{
    CursorPos prevPos = curPos;

    int maxLen = curBuffer->width - 3;
    char search[maxLen];
    int searchLen = 0;

    curBuffer->showMarkedLines = true;
    curBuffer->showCurrentLineMark = false;

    UiStatus status;
    while ((status = UiInputBox("Find", search, &searchLen, maxLen)) == UI_CONTINUE)
    {
        BufferUnmarkAll(curBuffer);
        CursorSetPos(curBuffer, prevPos.col, prevPos.row, false);
        CursorPos pos = curPos;

        bool found = false;
        if ((found = find(search, searchLen, 1, curRow, &pos)) == false)
            found = find(search, searchLen, 1, 0, &pos);

        if (found)
        {
            CursorPos p = {0, 0};
            while (find(search, searchLen, 1, p.row, &p))
            {
                BufferMarkLine(curBuffer, p.row, p.col, searchLen);
                p.row++;
            }
        }

        CursorSetPos(curBuffer, pos.col, pos.row, false);
        BufferCenterView(curBuffer);
        Render();
    }

    if (status == UI_CANCEL)
    {
        BufferUnmarkAll(curBuffer);
        CursorSetPos(curBuffer, prevPos.col, prevPos.row, false);
        BufferSetSearchWord(curBuffer, NULL, 0);
    }
    else
        BufferSetSearchWord(curBuffer, search, searchLen);

    curBuffer->showCurrentLineMark = true;
}
