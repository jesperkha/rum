#include "wim.h"

int main(int argc, char **argv)
{
    EditorInit();

    if (argc > 1)
        EditorOpenFile(argv[1]);
        
    while (1)
    {
        EditorHandleInput();
    }

    EditorExit();

    return EXIT_SUCCESS;
}
