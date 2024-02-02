#include "wim.h"

int main(int argc, char **argv)
{
    CmdOptions options = {0};
    if (!ProcessArgs(argc, argv, &options))
        return EXIT_FAILURE;

    EditorInit(options);

    while (1)
    {
        EditorHandleInput();
    }

    EditorExit();

    return EXIT_SUCCESS;
}
