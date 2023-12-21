#include "wim.h"

// Todo: steps before release v0.1.0
// Fix horizontal scroll
// Open file in start dir of cwd
// Remove path prefix from file name in statusbar
// Make branch without debug info

int main(int argc, char **argv)
{
    // Debug: clear log file
    FILE *f = fopen("log", "w");
    fclose(f);

    editorInit();

    // Open file
    if (argc > 1)
        editorOpenFile(argv[1]);

    while (1)
    {
        editorHandleInput();
    }

    editorExit();

    return EXIT_SUCCESS;
}
