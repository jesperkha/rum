#include "wim.h"

int main(int argc, char **argv)
{
    // Debug: clear log file
    FILE *f = fopen("log", "w");
    fclose(f);

    editorInit();

    // Open file
    if (argc > 1)
        editorLoadFile(argv[1]);

    while (1)
    {
        editorHandleInput();
    }

    editorExit();

    return EXIT_SUCCESS;
}
