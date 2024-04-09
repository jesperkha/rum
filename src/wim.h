#pragma once

#define TITLE "wim v0.4.0"

typedef unsigned char byte;

#define SYNTAX_NAME_LEN 16 // Length of extension name in syntax file
#define THEME_NAME_LEN 32  // Length of name in theme file
#define UNDO_CAP 256       // Max number of actions saved

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

#include "wim/types.h"
#include "wim/api.h"

#include "cmd.h"
#include "buffer.h"
#include "editor.h"
#include "util.h"
#include "screen.h"
