#include "editor.h"

int main(void)
{
    system("color");

    // Debug: clear log file
    FILE *f = fopen("log", "w");
    fclose(f);

    editorInit();
    bufferCreate();

    editorLoadFile("src/util.c");
    renderSatusBar();

    while (1)
    {
        editorHandleInput();
    }

    editorExit();

    return EXIT_SUCCESS;
}
