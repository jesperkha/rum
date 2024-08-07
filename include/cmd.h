#pragma once

typedef struct CmdOptions
{
    bool shouldExit;
    bool hasFile;
    bool rawMode;
    char filename[MAX_PATH];
} CmdOptions;

CmdOptions ProcessArgs(int argc, char **argv);