#pragma once

typedef struct CmdOptions
{
    bool hasFile;
    char filename[260];
} CmdOptions;

// Writes to op. Returns false on failure or if program should exit.
// Handles print commands like --help and --version.
bool ProcessArgs(int argc, char **argv, CmdOptions *op);