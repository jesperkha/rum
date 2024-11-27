// Build rum by compiling this file and running it
// gcc build.c && ./a.exe

#include <windows.h>
#include <stdio.h>

#define TARGET "rum.exe"
#define FLAGS "-Iinclude -DRELEASE -s -flto -O2 -o " TARGET

char command[2048] = {0};

void appendSourceFiles(const char *basePath)
{
    char searchPath[MAX_PATH];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", basePath);

    WIN32_FIND_DATAA file;
    HANDLE hFind = FindFirstFileA(searchPath, &file);

    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do
    {
        if (strcmp(file.cFileName, ".") != 0 && strcmp(file.cFileName, "..") != 0)
        {
            char newPath[MAX_PATH];
            snprintf(newPath, sizeof(newPath), "%s\\%s", basePath, file.cFileName);

            if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                appendSourceFiles(newPath);
            else
            {
                strcat(command, " ");
                strcat(command, newPath);
            }
        }
    } while (FindNextFile(hFind, &file) != 0);

    FindClose(hFind);
}

int main(void)
{
    printf("Building rum...\n");
    strcat(command, "gcc " FLAGS);
    appendSourceFiles("src");
    system(command);
    printf("Done! Created file %s\n", TARGET);
}