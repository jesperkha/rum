#pragma once

#include "version.h"
#define TITLE "rum v" VERSION

#ifdef _WIN32
#define OS_WINDOWS
#else
#define OS_UNKNOWN
#endif

typedef unsigned char byte;

#define SYNTAX_NAME_LEN 16 // Length of extension name in syntax file
#define THEME_NAME_LEN 32  // Length of name in theme file
#define UNDO_CAP 256       // Max number of actions saved
#define DEFAULT_TAB_SIZE 4

#define MAX_PATH 260 // Windows specific but used anyway

typedef enum Status
{
    RETURN_ERROR,
    RETURN_SUCCESS,
} Status;

#include <stdbool.h>
#include <string.h>

#include "types.h"
#include "api.h"

#include "cmd.h"
#include "buffer.h"
#include "editor.h"
#include "util.h"
#include "screen.h"
#include "os.h"
#include "log.h"
#include "syntax.h"