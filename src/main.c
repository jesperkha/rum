#include "rum.h"

int main(int argc, char **argv)
{
    CmdOptions options = ProcessArgs(argc, argv);
    if (options.shouldExit)
        return EXIT_FAILURE;

    EditorInit(options);

    while (true)
    {
        if (EditorHandleInput() != NIL)
            break;

        Render();
    }

    EditorFree();

    return EXIT_SUCCESS;
}
