#include "rum.h"

extern Editor editor;

String GetClipboardText()
{
    // Try to open the clipboard
    if (!OpenClipboard(NULL))
        return NULL_STRING;

    // Check if clipboard contains text
    if (!IsClipboardFormatAvailable(CF_TEXT))
    {
        CloseClipboard();
        return NULL_STRING;
    }

    // Get the clipboard data
    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == NULL)
    {
        CloseClipboard();
        return NULL_STRING;
    }

    // Lock the handle to get the actual text pointer
    char *pszText = (char *)GlobalLock(hData);
    if (pszText == NULL)
    {
        CloseClipboard();
        return NULL_STRING;
    }

    // Unlock the clipboard data
    GlobalUnlock(hData);

    // Close the clipboard
    CloseClipboard();

    int length = strlen(pszText);
    char *string = MemAlloc(length);
    memcpy(string, pszText, length);

    return (String){
        .s = string,
        .length = length,
        .null = false,
    };
}

void SetClipboardText(const char *text)
{
    // Open the clipboard
    if (!OpenClipboard(NULL))
        return;

    // Empty the clipboard to remove any existing content
    if (!EmptyClipboard())
    {
        CloseClipboard();
        return;
    }

    // Calculate the size of the string including the null terminator
    size_t len = strlen(text) + 1;

    // Allocate global memory for the text (CF_TEXT requires global memory)
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
    if (hMem == NULL)
    {
        CloseClipboard();
        return;
    }

    // Lock the memory and copy the string to it
    void *pMem = GlobalLock(hMem);
    if (pMem != NULL)
    {
        memcpy(pMem, text, len);
        GlobalUnlock(hMem);
    }
    else
    {
        GlobalFree(hMem);
        CloseClipboard();
        return;
    }

    // Set the clipboard data with CF_TEXT format
    if (SetClipboardData(CF_TEXT, hMem) == NULL)
        GlobalFree(hMem);

    // Close the clipboard
    CloseClipboard();

    // The clipboard now owns the memory, so we do not need to free it.
}

void PasteFromClipboard()
{
    String text = GetClipboardText();
    if (!text.null && text.length > 0)
    {
        TypingWriteMultiline(text.s, text.length);
        MemFree(text.s);
    }
}

void CopyToClipboard()
{
    char *text = BufferGetMarkedText(curBuffer);
    SetClipboardText(text);
}