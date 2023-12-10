#pragma once

enum
{
    HL_KEYWORD,
    HL_NUMBER,
    HL_STRING,
    HL_TYPE,
};

char *highlightLine(char *line, int length, int *newLength);