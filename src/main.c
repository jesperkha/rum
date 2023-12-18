#include "wim.h"

// Todo: steps before release v1.0.0
// 1. Fix horizontal scroll
// 2. Make branch without debug info

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
