#include "rum.h"

// Todo: Select and multi-line select
// Todo: C multi-line comments
// Todo: Run terminal commands from editor
// Todo: Comment out line/selection
// Todo: Save config changes at runtime (theme etc)

int main(int argc, char **argv)
{
    CmdOptions options = ProcessArgs(argc, argv);
    if (options.shouldExit)
        return EXIT_FAILURE;

    EditorInit(options);

    while (EditorHandleInput())
    {
        // ...
    }

    EditorFree();

    return EXIT_SUCCESS;
}
