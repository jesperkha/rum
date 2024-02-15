#include "wim.h"

// Todo: implement proper arg parsing

static void printHelp()
{
    printf("Usage: wim [filename] [options]      \n");
    printf("                                     \n");
    printf("Options:                             \n");
    printf("    -v --version   print version     \n");
    printf("    -h --help      display help menu \n");
}

bool ProcessArgs(int argc, char **argv, CmdOptions *op)
{
    if (argc > 2)
    {
        printHelp();
        return RETURN_ERROR;
    }

    if (argc == 2)
    {
        char *command = argv[1];

        if (!strcmp(command, "--version") || !strcmp(command, "-v"))
        {
            printf("%s\n", TITLE);
            return RETURN_ERROR;
        }
        else if (!strcmp(command, "--help") || !strcmp(command, "-h"))
        {
            printHelp();
            return RETURN_ERROR;
        }
        else if (command[0] == '-')
        {
            printf("Unkown option '%s' \n", command);
            printHelp();
            return RETURN_ERROR;
        }
        else
        {
            strncpy(op->filename, command, 260);
            op->hasFile = true;
        }
    }

    return RETURN_SUCCESS;
}