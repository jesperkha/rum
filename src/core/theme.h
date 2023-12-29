#pragma once

// \x1b[38;2;r;g;bm - foreground
// \x1b[48;2;r;g;bm - background
// #define BG(col) "\x1b[48;2;" col "m"
// #define FG(col) "\x1b[38;2;" col "m"
#define COL_RESET "\x1b[0m"

/*

    Make a color scheme

    #define COL_[name] "R;G;B"

    BG0 - editor background
    BG1 - statusbar and current line background
    BG2 - comments, line numbers

    FG0 - text

    YELLOW - function name
    RED    - keyword
    ORANGE - type name
    BLUE   - object
    AQUA   - operand symbol, macro
    GREY   - paren symbol
    PINK   - number
    GREEN  - string, char

*/

#define COL_BG0 (12 * 0)
#define COL_BG1 (12 * 1)
#define COL_BG2 (12 * 2)
#define COL_FG0 (12 * 3)

#define COL_YELLOW (12 * 4)
#define COL_BLUE (12 * 5)
#define COL_PINK (12 * 6)
#define COL_GREEN (12 * 7)
#define COL_AQUA (12 * 8)
#define COL_ORANGE (12 * 9)
#define COL_RED (12 * 10)
#define COL_GREY (12 * 11)

// https://github.com/morhetz/gruvbox
// https://draculatheme.com/contribute#color-palette
