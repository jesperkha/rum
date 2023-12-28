#pragma once

// Edit theme: (GRUVBOX, DRACULA)
#define DRACULA

// \x1b[38;2;r;g;bm - foreground
// \x1b[48;2;r;g;bm - background
#define BG(col) "\x1b[48;2;" col "m"
#define FG(col) "\x1b[38;2;" col "m"
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

#ifdef GRUVBOX

// https://github.com/morhetz/gruvbox

#define COL_BG0 "40;40;40"
#define COL_BG1 "60;56;54"
#define COL_BG2 "80;73;69"
#define COL_FG0 "235;219;178"

#define COL_YELLOW "215;153;33"
#define COL_RED "251;73;52"
#define COL_ORANGE "254;128;25"
#define COL_BLUE "131;165;152"
#define COL_GREY "146;131;116"
#define COL_AQUA "142;192;124"
#define COL_PINK "211;134;155"
#define COL_GREEN "185;187;38"

#endif
#ifdef DRACULA

// https://draculatheme.com/contribute#color-palette

#define COL_BG0 "40;42;54"
#define COL_BG1 "68;71;90"
#define COL_BG2 "98;114;164"
#define COL_FG0 "248;248;242"

#define COL_YELLOW "241;250;140"
#define COL_RED "255;85;85"
#define COL_ORANGE "255;184;108"
#define COL_BLUE "139;233;253"
#define COL_GREY "98;114;164"
#define COL_AQUA "139;233;253"
#define COL_PINK "255;121;198"
#define COL_GREEN "80;250;123"

#endif