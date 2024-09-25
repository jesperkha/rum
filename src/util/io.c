#include "rum.h"

extern Editor editor;

char *IoReadFile(const char *filepath, int *size)
{
    // Open file. EditorOpenFile does not create files and fails on file-not-found
    HANDLE file = CreateFileA(filepath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        Errorf("Failed to open file '%s'", filepath);
        return NULL;
    }

    // Get file size and read file contents into string buffer
    DWORD bufSize = GetFileSize(file, NULL) + 1;
    DWORD read;
    char *buffer = MemAlloc(bufSize);
    if (!ReadFile(file, buffer, bufSize, &read, NULL))
    {
        Errorf("Failed to read file '%s'", filepath);
        CloseHandle(file);
        return NULL;
    }

    CloseHandle(file);
    *size = bufSize - 1;
    buffer[bufSize - 1] = 0;
    return buffer;
}

bool IoWriteFile(const char *filepath, char *data, int size)
{
    // Open file - truncate existing and write
    HANDLE file = CreateFileA(filepath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        Error("failed to open file");
        return false;
    }

    DWORD written;
    if (!WriteFile(file, data, size, &written, NULL))
    {
        Error("failed to write to file");
        CloseHandle(file);
        return false;
    }

    CloseHandle(file);
    return true;
}
