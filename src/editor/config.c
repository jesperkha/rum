#include "rum.h"

#define wordSize 32 // Size of token lexemes

// Looks for files in the directory of the executable, eg. config, runtime etc.
// Returns pointer to file data, NULL on error. Writes to size. Remember to free!
char *ReadConfigFile(const char *file, int *size)
{
    const int pathSize = 512;

    // Concat path to executable with filepath
    char path[pathSize];
    int len = GetModuleFileNameA(NULL, path, pathSize);
    for (int i = len; i > 0 && path[i] != '\\'; i--)
        path[i] = 0;

    strcat(path, file);
    return IoReadFile(path, size);
}

typedef struct reader
{
    char *file;
    int size;
    int pos;
} reader;

Error readerFromFile(char *filepath, reader *r)
{
    int size;
    char *file = ReadConfigFile(filepath, &size);
    if (file == NULL || size == 0)
        return ERR_FILE_NOT_FOUND;

    r->file = file;
    r->size = size;
    r->pos = 0;
    return NIL;
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

        if (isString)
        {
            strncat(word, &c, 1);
            length++;
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
        if (isString)
            dest->type = T_STRING;
        else if (isNumber)
            dest->type = T_NUMBER;
        else if (!strcmp("true", word))
            dest->type = T_TRUE;
        else if (!strcmp("false", word))
            dest->type = T_FALSE;
        else
            Error("illegal token");

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
            Error("illegal symbol");

        strncpy(dest->word, r->file + r->pos, 1);
        dest->len = 1;
        r->pos++;
    }

    return true;
}

int expect_number(reader *r, token *t, int default_v)
{
    next(r, t); // Colon
    next(r, t); // Number

    if (t->type != T_NUMBER)
        Error("Expected number");

    int n = atoi(t->word);
    return n ? n : default_v;
}

bool expect_bool(reader *r, token *t)
{
    next(r, t); // Colon
    next(r, t); // Bool

    if (t->type != T_FALSE && t->type != T_TRUE)
        Error("Expected bool");

    return t->type == T_TRUE;
}

void expect_string(reader *r, token *t, char *dest)
{
    next(r, t); // Colon
    next(r, t); // String

    if (t->type != T_STRING)
        Error("Expected string");

    strncpy(dest, t->word, wordSize);
}

// Loads config file and writes to given config. Sets default config
// if file failed to open.
Error LoadConfig(Config *config)
{
    // Sane defaults
    config->tabSize = DEFAULT_TAB_SIZE;
    config->syntaxEnabled = true;
    config->matchParen = true;
    config->useCRLF = true;
    strcpy(config->theme, RUM_DEFAULT_THEME);

    reader r;
    token t;

    Error err = readerFromFile(RUM_CONFIG_FILEPATH, &r);
    if (err != NIL)
        return err;

#define isword(w) (!strncmp(t.word, w, wordSize))

    next(&r, &t); // RBRACE

    while (next(&r, &t))
    {
        if (t.type == T_STRING)
        {
            if (isword("tabSize"))
                config->tabSize = expect_number(&r, &t, DEFAULT_TAB_SIZE);
            else if (isword("useCRLF"))
                config->useCRLF = expect_bool(&r, &t);
            else if (isword("matchParen"))
                config->matchParen = expect_bool(&r, &t);
            else if (isword("theme"))
                expect_string(&r, &t, config->theme);
            else if (isword("syntaxEnabled"))
                config->syntaxEnabled = expect_bool(&r, &t);
            else
                Errorf("Unknown key %s", t.word);
            continue;
        }

        if (t.type == T_RBRACE)
            break;

        if (t.type == T_COMMA)
            continue;

        Error("unhandled case in json parsing");
    }

    if (t.type != T_RBRACE)
        return ERR_CONFIG_PARSE_FAIL;

    MemFree(r.file);
    Log("Config loaded");
    return NIL;
}

// Writes rgb value to dest. Sets default value and returns false if src is invalid.
// Size of dest should be at least 12 including NULL terminator.
bool hex_to_rgb(char *src, char *dest, char *default_v)
{
    if (strlen(src) != 6)
        goto fail;

    int n = 0;
    long nums[3];

    for (int i = 0; i < 6; i += 2)
    {
        char hex[3] = {0};
        strncpy(hex, src + i, 2);

        if (!isalnum(hex[0]) || !isalnum(hex[1]))
            goto fail;

        nums[n++] = strtol(hex, NULL, 16);
    }

    // Cannot be bigger than 255 so long is fine
    sprintf_s(dest, 16, "%03ld;%03ld;%03ld", nums[0], nums[1], nums[2]);
    return true;

fail:
    strcpy(dest, default_v);
    return false;
}

// Loads theme data into colors.
Error LoadTheme(char *name, Colors *colors)
{
    char path[128];
    sprintf_s(path, 128, "./config/themes/%s.json", name);

    reader r;
    token t;

    Error err = readerFromFile(path, &r);
    if (err != NIL)
        return err;

    next(&r, &t); // RBRACE

    while (next(&r, &t))
    {
        if (t.type == T_COMMA)
            continue;
        if (t.type == T_RBRACE)
            break;

        if (t.type != T_STRING)
        {
            Error("expected string");
            return ERR_CONFIG_PARSE_FAIL;
        }

        char name[wordSize];
        strncpy(name, t.word, wordSize);
        char colorHex[wordSize];
        expect_string(&r, &t, colorHex);

        char colorRGB[32] = {0};
        if (!hex_to_rgb(colorHex, colorRGB, "0;0;0"))
            return ERR_CONFIG_PARSE_FAIL;

#define set_color(n, dest)                   \
    if (!strncmp(n, name, wordSize))         \
    {                                        \
        memset(dest, 0, COLOR_SIZE);         \
        strncpy(dest, colorRGB, COLOR_SIZE); \
        continue;                            \
    }

        set_color("bg0", colors->bg0);
        set_color("bg1", colors->bg1);
        set_color("bg2", colors->bg2);
        set_color("fg0", colors->fg0);
        set_color("symbol", colors->symbol);
        set_color("object", colors->object);
        set_color("bracket", colors->bracket);
        set_color("number", colors->number);
        set_color("string", colors->string);
        set_color("type", colors->type);
        set_color("keyword", colors->keyword);
        set_color("function", colors->function);
        set_color("userType", colors->userType);

        Errorf("unknown color name: %s", name);
    }

    if (t.type != T_RBRACE)
        return ERR_CONFIG_PARSE_FAIL;

    strncpy(colors->name, name, 31);
    MemFree(r.file);
    Log("Theme loaded");
    return NIL;
}
