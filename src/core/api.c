#include "core.h"

static bool prevented = false;

void PreventDefault()
{
    prevented = true;
}

// Returns true if defualt action should be prevented.
bool apiOnInput(InputInfo *info)
{
    prevented = false;
    WimInputRecord apiRecord;
    mempcpy(&apiRecord, &info, sizeof(apiRecord));
    onInput(&apiRecord);
    return prevented;
}