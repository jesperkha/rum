#pragma once

#include "version.h"
#define TITLE "rum v" VERSION

typedef unsigned char byte;

// n kilobytes
#define KB(n) ((n) * 1024)
// n megabytes
#define MB(n) (KB(n) * 1024)

#define RENDER_BUFFER_SIZE KB(100)

#define SYNTAX_NAME_LEN 16 // Length of extension name in syntax file
#define THEME_NAME_LEN 32  // Length of name in theme file
#define UNDO_CAP 64        // Max number of actions saved. Todo: make undo size dynamic
#define DEFAULT_TAB_SIZE 4

#define MAX_PATH 260 // Windows specific but used anyway

typedef enum Status
{
    RETURN_ERROR,
    RETURN_SUCCESS,
} Status;

#include <windows.h>
#include <stdbool.h>
#include <string.h>

#include "types.h"
#include "api.h"
#include "cmd.h"
#include "buffer.h"
#include "editor.h"
#include "util.h"
#include "screen.h"
#include "log.h"
#include "syntax.h"