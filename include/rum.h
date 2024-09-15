#pragma once

#include "version.h"
#define TITLE "rum v" VERSION

#define KB(n) ((n) * 1024)       // n kilobytes
#define MB(n) (KB(n) * 1024)     // n megabytes
#define RENDER_BUFFER_SIZE MB(1) // Constant max size of buffer used for rendering

#define SYNTAX_NAME_LEN 16         // Length of extension name in syntax file
#define THEME_NAME_LEN 32          // Length of name in theme file
#define DEFAULT_TAB_SIZE 4         // Defaults to this if config not found
#define BUFFER_DEFAULT_LINE_CAP 32 // Buffers are created with this defualt cap
#define LINE_DEFAULT_LENGTH 32     // Default raw line length in buffer
#define UNDO_DEFUAULT_CAP 128      // Default max number of undos in list before realloc
#define EDITOR_ACTION_BUFSIZE 16   // Character cap for string in action.
#define SYNTAX_COMMENT_SIZE 8      // Max size of comment string
#define FILE_EXTENSION_SIZE 16     // Max length of file extension name
#define MAX_PATH 260               // Windows specific but used anyway
#define MAX_SEARCH 64              // Max search string in buffer
#define COLOR_SIZE 13              // Size of a color string including NULL
#define COLOR_BYTE_LENGTH 19       // Number of bytes in a color sequence
#define EDITOR_BUFFER_CAP 16       // Max number of buffers that can be open at one time, not dymamic
#define PAD_BUFFER_SIZE 512        // Size of padding buffer

#define RUM_CONFIG_FILEPATH "config/config.json"
#define RUM_DEFAULT_THEME "dracula"

typedef enum Error
{
    NIL = 1, // !func() is not allowed

    ERR_EXIT,
    ERR_FILE_NOT_FOUND,
    ERR_FILE_SAVE_FAIL,
    ERR_CONFIG_PARSE_FAIL,
    ERR_INPUT_READ_FAIL,
} Error;

#include <windows.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "types.h"
#include "api.h"
#include "cmd.h"
#include "buffer.h"
#include "editor.h"
#include "util.h"
#include "screen.h"
#include "log.h"
#include "syntax.h"