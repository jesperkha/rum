#include "wim.h"

// Defined in editor.c
char *readFile(const char *filepath, int *size);

#define wordSize 32
typedef struct reader
{
    char *file;
    int size;
    int pos;

    char key[wordSize];
    char val[wordSize];
} reader;

// Writes word to dest with max size. Returns false on end of string.
bool next_word(reader *r, char *dest)
{
    char *start = r->file + r->pos;
    int count = 0;

    while (r->pos < r->size)
    {
        char c = r->file[r->pos];
        if (
            c == 0 ||
            c == ' ' ||
            c == '\n' ||
            c == '\r' ||
            c == '\t')
            break;

        r->pos++;
        count++;
    }

    if (r->file[r->pos] != 0 && count == 0)
    {
        r->pos++;
        return next_word(r, dest);
    }

    strncpy(dest, start, count);
    return count > 0;
}

// Sets the key and val of reader with next key-value pair. Returns false on fail.
bool read_next(reader *r)
{
    memset(r->key, 0, wordSize);
    memset(r->val, 0, wordSize);
    return next_word(r, r->key) && next_word(r, r->val);
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
    char *path = "C:/Users/hamme/wim/config/config.wim";
    LogString("Config path", path);

    int size;
    char *file = readFile(path, &size);
    if (file == NULL || size == 0)
        return RETURN_ERROR;

    reader r = {
        .file = file,
        .size = size,
        .pos = 0,
    };

    // Sane defaults
    config->tabSize = DEFAULT_TAB_SIZE;
    config->syntaxEnabled = true;
    config->matchParen = true;
    config->useCRLF = true;

    while (read_next(&r))
    {
        if (!strcmp(r.key, "tabSize"))
            config->tabSize = parse_int(r.val, DEFAULT_TAB_SIZE);
        else if (!strcmp(r.key, "syntaxEnabled"))
            config->syntaxEnabled = parse_bool(r.val, true);
        else if (!strcmp(r.key, "matchParen"))
            config->matchParen = parse_bool(r.val, true);
        else
            LogString("Unknown config param", r.key);
    }

    MemFree(file);
    return RETURN_SUCCESS;
}

// Writes rgb value to dest. Sets default value and returns false if src is invalid.
// Size of dest should be at least 12 including NULL terminator.
bool hex_to_rgb(char *src, char *dest, char *default_v)
{
    if (strlen(src) != 7)
        goto fail;

    int n = 0;
    long nums[3];

    for (int i = 1; i < 7; i += 2)
    {
        char hex[3] = {0};
        strncpy(hex, src + i, 2);

        if (!isalnum(hex[0]) || !isalnum(hex[1]))
            goto fail;

        nums[n++] = strtol(hex, NULL, 16);
    }

    // Cannot be bigger than 255 so long is fine
    sprintf_s(dest, 16, "%ld;%ld;%ld", nums[0], nums[1], nums[2]);
    return true;

fail:
    strcpy(dest, default_v);
    return false;
}

// Loads theme data into colors.
Status LoadTheme(char *name, Colors *colors)
{
    char path[128];
    sprintf_s(path, 128, "C:/Users/hamme/wim/config/themes/%s.wim", name);
    LogString("Theme path", path);

    int size;
    char *file = readFile(path, &size);
    if (file == NULL || size == 0)
        return RETURN_ERROR;

    reader r = {
        .file = file,
        .size = size,
        .pos = 0,
    };

#define set_color(name, field)        \
    if (!strcmp(r.key, name))         \
    {                                 \
        memset(field, 0, COLOR_SIZE); \
        strcpy(field, color);         \
        continue;                     \
    }

    while (read_next(&r))
    {
        char color[32] = {0};
        if (!hex_to_rgb(r.val, color, "0;0;0"))
            return RETURN_ERROR;

        set_color("bg0", colors->bg0);
        set_color("bg1", colors->bg1);
        set_color("bg2", colors->bg2);
        set_color("fg0", colors->fg0);
        set_color("aqua", colors->aqua);
        set_color("blue", colors->blue);
        set_color("gray", colors->gray);
        set_color("pink", colors->pink);
        set_color("green", colors->green);
        set_color("orange", colors->orange);
        set_color("red", colors->red);
        set_color("yellow", colors->yellow);
    }

    strncpy(colors->name, name, 31);
    MemFree(file);
    return RETURN_SUCCESS;
}

void LoadSyntax()
{
}