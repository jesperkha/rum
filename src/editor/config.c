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

// Writes file contents to reader.
Status readerFromFile(char *filepath, reader *r)
{
    int size;
    char *file = readFile(filepath, &size);
    if (file == NULL || size == 0)
        return RETURN_ERROR;

    r->file = file;
    r->size = size;
    r->pos = 0;
    return RETURN_SUCCESS;
}

enum TokenTypes
{
    T_LBRACE,
    T_RBRACE,
    T_LSQUARE,
    T_RSQUARE,
    T_COMMA,
    T_COLON,

    T_STRING,
    T_NUMBER,
    T_TRUE,
    T_FALSE,
};

typedef struct token
{
    char word[wordSize];
    int len;
    int type;
} token;

// Reads next token from file and writes to dest. Returns true on success.
bool next(reader *r, token *dest)
{
    memset(dest->word, 0, wordSize);
    char word[wordSize] = {0};
    int length = 0;
    bool isNumber = false;
    bool isString = false;

    for (int i = r->pos; i < r->size; i++)
    {
        if (length >= wordSize)
            goto write_token;

        char c = r->file[i];

        if (c == '"')
        {
            if (isString)
            {
                r->pos += 2;
                goto write_token;
            }
            isString = true;
            continue;
        }

        if (!isalnum(c))
            goto write_token;

        strncat(word, &c, 1);
        length++;

        if (length == 1 && isdigit(c))
            isNumber = true;
    }

    // If we havent jumped then EOF
    return false;

write_token:
    if (length > 0)
    {
        if (isNumber)
            dest->type = T_NUMBER;
        else if (isString)
            dest->type = T_STRING;
        else if (!strcmp("true", word))
            dest->type = T_TRUE;
        else if (!strcmp("false", word))
            dest->type = T_FALSE;
        else
            LogError("illegal token");

        dest->len = length;
        strncpy(dest->word, word, wordSize);
        r->pos += length;
    }
    else
    {
        char c = r->file[r->pos];
        if (strchr(" \n\r\t", c) != NULL)
        {
            r->pos++;
            return next(r, dest);
        }
        else if (c == '{')
            dest->type = T_LBRACE;
        else if (c == '}')
            dest->type = T_RBRACE;
        else if (c == '[')
            dest->type = T_LSQUARE;
        else if (c == ']')
            dest->type = T_RSQUARE;
        else if (c == ':')
            dest->type = T_COLON;
        else if (c == ',')
            dest->type = T_COMMA;
        else
            LogError("illegal symbol");

        strncpy(dest->word, r->file + r->pos, 1);
        dest->len = 1;
        r->pos++;
    }

    return true;
}

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
            c == '=' ||
            c == '"' ||
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

int expect_number(reader *r, token *t, int default_v)
{
    next(r, t); // Colon
    next(r, t); // Number

    if (t->type != T_NUMBER)
        LogError("Expected number");

    int n = atoi(t->word);
    return n ? n : default_v;
}

bool expect_bool(reader *r, token *t, bool default_v)
{
    next(r, t); // Colon
    next(r, t); // Bool

    if (t->type != T_FALSE && t->type != T_TRUE)
        LogError("Expected bool");

    return t->type == T_TRUE;
}

// Loads config file and writes to given config. Sets default config
// if file failed to open.
Status LoadConfig(Config *config)
{
    // Sane defaults
    config->tabSize = DEFAULT_TAB_SIZE;
    config->syntaxEnabled = true;
    config->matchParen = true;
    config->useCRLF = true;

    reader r;
    if (!readerFromFile("config/config.json", &r))
        return RETURN_ERROR;

    token t;

#define isword(w) (!strncmp(t.word, w, wordSize))

    expect(T_LBRACE);
    while (next(&r, &t))
    {
        if (t.type == T_STRING)
        {
            if (isword("tabSize"))
                config->tabSize = expect_number(&r, &t, DEFAULT_TAB_SIZE);
            else if (isword("useCRLF"))
                config->useCRLF = expect_bool(&r, &t, true);
            else if (isword("matchParen"))
                config->matchParen = expect_bool(&r, &t, true);
            else if (isword("syntaxEnabled"))
                config->syntaxEnabled = expect_bool(&r, &t, true);
            else
                LogString("Unknown key", t.word);
            continue;
        }

        if (t.type == T_RBRACE)
            break;

        if (t.type == T_COMMA)
            continue;

        LogError("unhandled case in json parsing");
    }

    if (t.type != T_RBRACE)
        return RETURN_ERROR;

    MemFree(r.file);
    Log("Config loaded");
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
    sprintf_s(path, 128, "./config/themes/%s.toml", name);
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

        LogError("Invalid color key");
    }

    strncpy(colors->name, name, 31);
    MemFree(file);
    return RETURN_SUCCESS;
}

/*

filetype: C
extensions: c, h, cpp, hpp
single-comment: //
multi-comment-start:
multi-comment-end:

keywords:

...

types:

...

*/

void LoadSyntax()
{
}