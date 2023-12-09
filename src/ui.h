#pragma once

enum
{
    UI_YES,
    UI_NO,
    UI_OK,
    UI_CANCEL,
};

int uiPromptYesNo(const char *message);
int uiTextInput(int x, int y, char *buffer, int size);