#include "core.h"

int main(int argc, char **argv)
{
    editorInit();

    if (argc > 1)
        editorOpenFile(argv[1]);
    
    while (1)
    {
        editorHandleInput();
    }

    editorExit();

    return EXIT_SUCCESS;
}
