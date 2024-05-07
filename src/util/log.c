#include "wim.h"

#define LOG_FILE "log"

#ifdef DEBUG

void _Log(char *message, char *filepath, int lineNumber)
{
    FILE *f = fopen(LOG_FILE, "a");
    if (f != NULL)
    {
        fprintf(f, "%20s:%-3d [LOG] %s\n", filepath, lineNumber, message);
        fclose(f);
    }
}

void _LogNumber(char *message, int number, char *filepath, int lineNumber)
{
    FILE *f = fopen(LOG_FILE, "a");
    if (f != NULL)
    {
        fprintf(f, "%20s:%-3d [LOG] %s: %d\n", filepath, lineNumber, message, number);
        fclose(f);
    }
}

void _LogString(char *message, char *str, char *filepath, int lineNumber)
{
    FILE *f = fopen(LOG_FILE, "a");
    if (f != NULL)
    {
        fprintf(f, "%20s:%-3d [LOG] %s: %s\n", filepath, lineNumber, message, str);
        fclose(f);
    }
}

void _LogError(char *message, char *filepath, int lineNumber)
{
    FILE *f = fopen(LOG_FILE, "a");
    if (f != NULL)
    {
        fprintf(f, "%20s:%-3d [ERR] %s\n", filepath, lineNumber, message);
        fclose(f);
    }
}

void LogCreate()
{
    FILE *f = fopen(LOG_FILE, "w");
    fclose(f);
}

#endif