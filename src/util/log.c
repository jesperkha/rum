#include "wim.h"

#define LOG_FILE "wimlog"

#ifdef DEBUG

void _Log(char *message, char *filepath, int lineNumber)
{
    FILE *f = fopen(LOG_FILE, "a");
    if (f != NULL)
    {
        fprintf(f, "LOG at %s, line %d: %s\n", filepath, lineNumber, message);
        fclose(f);
    }
}

void _LogNumber(char *message, int number, char *filepath, int lineNumber)
{
    FILE *f = fopen(LOG_FILE, "a");
    if (f != NULL)
    {
        fprintf(f, "LOG at %s, line %d: %s, %d\n", filepath, lineNumber, message, number);
        fclose(f);
    }
}

void _LogError(char *message, char *filepath, int lineNumber)
{
    FILE *f = fopen(LOG_FILE, "a");
    if (f != NULL)
    {
        fprintf(f, "ERROR at %s, line %d: %s\n", filepath, lineNumber, message);
        fclose(f);
    }
}

void LogCreate()
{
    FILE *f = fopen(LOG_FILE, "w");
    fclose(f);
}

#endif