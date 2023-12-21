# wim - Windows Vim

Vim-like terminal editor using the win32 api for Windows.

Wim has no other dependencies than libc and the win32 api. Currently, it offers basic text editing, along some quality of life features like parenthesis mathing and dynamic tabs. Wim also features syntax highlighting for C files. See [roadmap.md](roadmap.md).

Build with **make**. Usage: `wim [filename]`

### Demo

![Demo of wim editor](.github/demo.gif)

### Controls:

- `ctrl-q`: Exit wim. Pressing the escape key will do the same.
- `ctrl-c`: Enter command line (exit with ESC). Type one of the following commands:
    - `:exit`: Exit wim
    - `:save`: Save file
    - `:open`: Open file
- `ctrl-s`: Save buffer to file. If a file is opened it simply saves, if the buffer has no filename it prompts the user for a name.
- `ctrl-o`: Shortcut for `:open` command
- `ctrl-n`: Create new empty buffer
- `ctrl-x`: Delete line