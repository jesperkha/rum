#include "wim.h"

// Gets filename, including extension, from filepath
void str_filename(char *dest, char *src)
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
void str_fextension(char *dest, char *src)
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
char *str_memstr(char *buf, char *substr, size_t size)
{
    size_t sublen = strlen(substr);
    int i = 0;
    while (i < size)
    {
        if (i + sublen > size)
            return NULL;
        // Todo: memcmp returns early making single char substrings valid
        if (!memcmp(buf + i, substr, sublen))
            return buf + i;
        i++;
    }

    return NULL;
}