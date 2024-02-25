#include "wim.h"

#ifdef DEBUG

void Log(char *message)
{
    FILE *f = fopen("log", "a");
    if (f != NULL)
    {
        fprintf(f, "[ LOG ]: %s\n", message);
        fclose(f);
    }
}

void LogNumber(char *message, int number)
{
    FILE *f = fopen("log", "a");
    if (f != NULL)
    {
        fprintf(f, "[ LOG ]: %s: %d\n", message, number);
        fclose(f);
    }
}

void LogError(char *message)
{
    FILE *f = fopen("log", "a");
    if (f != NULL)
    {
        fprintf(f, "[ ERROR ]: %s\n", message);
        fclose(f);
    }
}

#endif