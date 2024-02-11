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