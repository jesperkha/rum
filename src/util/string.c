#include "rum.h"

int StrCount(char *s, char c)
{
    int n = 0;
    while (*s != 0)
    {
        if (*s == c)
            n++;
        s++;
    }
    return n;
}

// Caps width of string by replacing spaces with newlines. Source is modified.
void StrCapWidth(char *source, int maxW)
{
    char *p = source;
    char *last, *linebegin = p;

    // Cooked thoroughly at 2 am

    while (*p != 0)
    {
        if (*p == ' ')
        {
            if (p - linebegin > maxW && last != linebegin)
            {
                *last = '\n';
                p = last + 1;
                linebegin = p;
            }
            last = p;
        }
        else if (*p == '\n')
        {
            linebegin = p;
            last = p;
        }
        p++;
    }

    if ((source + strlen(source)) - linebegin > maxW)
        if ((p = strrchr(source, ' ')) != NULL)
            *p = '\n';
}

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

// Modifies path if add ~ at beginning if path is in home directory.
// Returns pointer to beginning of short path, which is withing path.
// If the path is not within home, the returned pointer is just path.
char *StrGetShortPath(char *path)
{
    char homepathCmd[32];

    char *homepath = getenv("HOME"); // bash (C:/Users/<usr>)
    if (homepath == NULL)            // cmd (Users/<usr>)
    {
        homepath = getenv("HOMEPATH");
        AssertNotNull(homepath);

        sprintf(homepathCmd, "C:%s", homepath);
        homepath = homepathCmd;
    }

    int n = strlen(homepath);

    StrReplace(path, '\\', '/');
    StrReplace(homepath, '\\', '/');

    if (!strncmp(homepath, path, n))
    {
        path[n - 1] = '~';
        if (path[n] != 0)
            path[n] = '/';
        return path + n - 1;
    }

    return path;
}

void StrReplace(char *s, char find, char replace)
{
    int i = 0;
    while (s[i] != 0)
    {
        if (s[i] == find)
            s[i] = replace;
        i++;
    }
}

#define kilo (1000)
#define mega (kilo * 1000)
#define giga (mega * 1000)

// Converts n to a readable format: 18200 -> 18K etc. Unsafe.
void StrNumberToReadable(unsigned long long n, char *dest)
{
    if (n > giga)
        sprintf(dest, "%3dG", (int)(n / giga));
    else if (n > mega)
        sprintf(dest, "%3dM", (int)(n / mega));
    else if (n > kilo)
        sprintf(dest, "%3dK", (int)(n / kilo));
    else
        sprintf(dest, " %3d", (int)n);
}

// Returns true if c is a printable ascii character
bool isChar(char c)
{
    return c >= 32 && c <= 126;
}