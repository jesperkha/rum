#include "rum.h"

extern Editor editor;
extern Colors colors;
extern Config config;

// Buffer written to and rendered with highlights.
// Note: may cause segfault with long ass lines.
#define HL_BUFSIZE 2048
char hlBuffer[HL_BUFSIZE];

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

    switch (b->fileType)
    {
    case FT_C:
        langC(line, &iter, &cb);
        break;

    case FT_PYTHON:
        langPy(line, &iter, &cb);
        break;

    case FT_JSON:
        langJson(line, &iter, &cb);
        break;

    default:
        return line;
    }

    if (line.isCurrentLine && !b->showHighlight && b->showCurrentLineMark)
        CbColor(&cb, colors.bg1, colors.fg0);
    else
        CbColor(&cb, colors.bg0, colors.fg0);

    return (HlLine){
        .length = CbLength(&cb),
        .line = cb.buffer,
        .rawLength = line.length,
        .row = line.row,
        .isCurrentLine = line.isCurrentLine,
    };
}
