#include "wim.h"

// Gets filename, including extension, from filepath
void StrFilename(char *dest, char *src)
{
    char *slash = src;
    for (int i = strlen(src); i >= 0; i--)
    {
        if (src[i] == '/' || src[i] == '\\')
            break;

        slash = src + i;
    }

    strcpy(dest, slash);
}

// Gets the file extension, excluding the peroid.
void StrFileExtension(char *dest, char *src)
{
    char *dot = src;
    for (int i = strlen(src); i >= 0; i--)
    {
        if (src[i] == '.')
            break;

        dot = src + i;
    }

    strcpy(dest, dot);
}

// Returns pointer to first character in first instance of substr in buf. NULL if none is found.
char *StrMemStr(char *buf, char *substr, size_t size)
{
    size_t sublen = strlen(substr);
    int i = 0;
    while (i < size)
    {
        if (i + sublen > size)
            return NULL;
        if (!memcmp(buf + i, substr, sublen))
            return buf + i;
        i++;
    }

    return NULL;
}

// Returns true if c is a printable ascii character
bool isChar(char c)
{
    return c >= 32 && c <= 126;
}