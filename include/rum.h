#pragma once

#include "version.h"
#define TITLE "rum v" VERSION

typedef unsigned char byte;

#define SYNTAX_NAME_LEN 16 // Length of extension name in syntax file
#define THEME_NAME_LEN 32  // Length of name in theme file
#define UNDO_CAP 256       // Max number of actions saved

#define DEFAULT_TAB_SIZE 4

typedef enum Status
{
    RETURN_ERROR,
    RETURN_SUCCESS,
} Status;

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>

#include "types.h"
#include "api.h"

#include "cmd.h"
#include "buffer.h"
#include "editor.h"
#include "util.h"
#include "log.h"
#include "screen.h"
