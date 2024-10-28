#include "rum.h"

typedef void (*PlugInitFunc)(void);

// Loads plugin DLLs and puts init functions into results array.
// Returns number of results. Returns -1 on error.
int LoadPlugins(char *pluginDir, PlugInitFunc *results, size_t maxResults)
{
    SetCurrentDirectoryA(pluginDir);

    WIN32_FIND_DATAA data;
    HANDLE hfind = FindFirstFileA("./*", &data);
    if (hfind == INVALID_HANDLE_VALUE)
        return -1;

    int count = 0;

    do
    {
        char *filename = data.cFileName;
        size_t length = strlen(filename);

        if (length <= 4 || strcmp(filename + (length - 4), ".dll"))
            continue;

        HMODULE hplugin = LoadLibraryA(filename);
        if (hplugin == NULL)
        {
            Errorf("Failed to load plugin %s", filename);
            continue;
        }

        PlugInitFunc f = (PlugInitFunc)GetProcAddress(hplugin, "PlugInit");
        if (f == NULL)
        {
            Errorf("Cannot find init func in plugin %s", filename);
            continue;
        }

        results[count] = f;
        count++;
    } while (FindNextFileA(hfind, &data) != 0 && count < (int)maxResults);

    return count;
}
