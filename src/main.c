#include "rum.h"

int main(int argc, char **argv)
{
    CmdOptions options = ProcessArgs(argc, argv);
    if (options.shouldExit)
        return EXIT_FAILURE;

    EditorInit(options);

    while (EditorHandleInput() == NIL)
    {
        // ...
    }

    EditorFree();

    return EXIT_SUCCESS;
}
