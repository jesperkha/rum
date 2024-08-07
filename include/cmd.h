#pragma once

typedef struct CmdOptions
{
    bool shouldExit;
    bool hasFile;
    char filename[MAX_PATH];
} CmdOptions;

CmdOptions ProcessArgs(int argc, char **argv);