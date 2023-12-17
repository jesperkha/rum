#include "wim.h"

// Todo: steps before release v1.0.0
// 1. Open new empty file
// 2. Fix horizontal scroll

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
