#include "editor.h"
#include "ui.h"

int main(int argc, char **argv)
{
    // Debug: clear log file
    FILE *f = fopen("log", "w");
    fclose(f);

    editorInit();
    bufferInit();

    uiPromptYesNo("Is wim the coolest text editor?");

    renderSatusBar("[empty file]");
    if (argc > 1)
    {
        // Open file
        editorLoadFile(argv[1]);
        renderSatusBar(argv[1]);
    }

    while (1)
    {
        editorHandleInput();
    }

    editorExit();

    return EXIT_SUCCESS;
}
