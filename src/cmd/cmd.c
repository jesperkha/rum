#include "rum.h"

static void printHelp()
{
    printf("\n");
    printf("Usage: rum <filename> <options...>   \n");
    printf("                                     \n");
    printf("Options:                             \n");
    printf("    -v --version   print version     \n");
    printf("    -h --help      display help menu \n");
}

CmdOptions ProcessArgs(int argc, char **argv)
{
    CmdOptions err = {.shouldExit = true};
    CmdOptions ops = {0};

    for (int i = 1; i < argc; i++)
    {
#define is(_cmd) (!strcmp((_cmd), argv[i]))
        char *arg = argv[i];

        if (is("--version") || is("-v"))
        {
            printf("%s\n", TITLE);
            return err;
        }

        if (is("--help") || is("-h"))
        {
            printHelp();
            return err;
        }

        if (arg[0] == '-')
        {
            printf("Error: Unkown option '%s' \n", arg);
            printHelp();
            return err;
        }

        if (ops.hasFile)
        {
            printf("Error: Too many input files\n");
            return err;
        }

        strncpy(ops.filename, arg, MAX_PATH);
        ops.hasFile = true;
    }

    return ops;
}