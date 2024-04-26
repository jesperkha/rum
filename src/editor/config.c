#include "wim.h"

// Defined in editor.c
char *readFile(const char *filepath, int *size);

typedef struct reader
{
    char *file;
    int size;
    int pos;
} reader;

// Writes word to dest with max size. Returns false on end of string.
bool nextword(reader *r, char *dest, int size)
{
    char *start = r->file + r->pos;
    int count = 0;

    while (r->pos < r->size)
    {
        char c = r->file[r->pos];
        if (c == 0 || c == ' ' || c == '\n')
            break;
        
        r->pos++;
        count++;
    }

    if (r->file[r->pos] != 0 && count == 0)
    {
        r->pos++;
        return nextword(r, dest, size);
    }

    strncpy(dest, start, count);
    return count > 0;
}

bool parse_bool(char *word, bool default_v)
{
    if (!strcmp(word, "true"))
        return true;
    else if (!strcmp(word, "false"))
        return false;
    return default_v;
}

int parse_int(char *word, int default_v)
{
    int n = atoi(word);
    return n ? n : default_v;
}

// Loads config file and writes to given config. Sets default config
// if file failed to open.
Status LoadConfig(Config *config)
{
    int numPaths = 1;
    char *tryPaths[1] = {
        "C:/Users/hamme/wim/temp/config.wim",
    };

    int size;
    char *file;
    for (int i = 0; i < numPaths; i++)
    {
        file = readFile(tryPaths[i], &size);
        if (file != NULL)
        {
            LogString("Config file loaded", tryPaths[i]);
            break;
        }
    }

    if (file == NULL)
    {
        LogError("Failed to load config file");
        return RETURN_ERROR;
    }

    reader r = {
        .file = file,
        .size = size,
        .pos = 0,
    };

    #define bufSize 32
    char key[bufSize] = {0};
    char val[bufSize] = {0};

    // Sane defaults
    config->tabSize = DEFAULT_TAB_SIZE;
    config->syntaxEnabled = true;
    config->matchParen = true;

    while (nextword(&r, key, bufSize) && nextword(&r, val, bufSize))
    {
        if (!strcmp(key, "tabSize"))
            config->tabSize = parse_int(val, DEFAULT_TAB_SIZE);
        else if (!strcmp(key, "syntaxEnabled"))
            config->syntaxEnabled = parse_bool(val, true);
        else if (!strcmp(key, "matchParen"))
            config->matchParen = parse_bool(val, true);
        else {
            LogError("Unknown config param");
        }

        memset(key, 0, bufSize);
        memset(val, 0, bufSize);
    }

    MemFree(file);
    return RETURN_SUCCESS;
}

void LoadThemes()
{

}

void LoadSyntax()
{

}