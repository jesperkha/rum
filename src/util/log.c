#include "wim.h"

void Log(char *message)
{
#ifdef DEBUG
    FILE *f = fopen("log", "a");
    if (f != NULL)
    {
        fprintf(f, "[ LOG ]: %s\n", message);
        fclose(f);
    }
#endif
}

void LogNumber(char *message, int number)
{
#ifdef DEBUG
    FILE *f = fopen("log", "a");
    if (f != NULL)
    {
        fprintf(f, "[ LOG ]: %s: %d\n", message, number);
        fclose(f);
    }
#endif
}

void LogError(char *message)
{
#ifdef DEBUG
    FILE *f = fopen("log", "a");
    if (f != NULL)
    {
        fprintf(f, "[ ERROR ]: %s\n", message);
        fclose(f);
    }
#endif
}
