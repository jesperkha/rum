<br />
<div align="center">
  <img src=".github/logo.svg" alt="Logo" width="120">

  <h4 align="center">Simple terminal editor for the windows console.</h4>

  <p align="center">
    Currently supports basic text editing, syntax
    highlighting, typing features such <br> as parenthesis matching and dynamic tabs, and config files for syntax and themes.
    <br />
    <a href="https://github.com/jesperkha/wim/releases/tag/v0.1.0"><strong>Latest release »</strong></a>
    <br />
    <br />
  </p>
</div>

## About

Wim is a soon-to-be vim-like terminal editor made for the Windows terminal, using the win32 console API. Wim has no other dependencies than libc and win32. See [roadmap.md](roadmap.md) for progress on development.

Build with make. Usage: `wim [filename]`

**Note:** When moving the executable to another location, make sure you copy the `runtime` directory along with it.

## Screenshots

<div align="center">
<img src=".github/screenshot.png" alt="Screenshot" width="90%">

<a href="https://github.com/jesperkha/wim/blob/main/.github/demo.gif">Demo gif</a>
</div>


## Controls:

- `ctrl-q`: Exit wim. Pressing the escape key will do the same.
- `ctrl-c`: Enter command line (exit with ESC). Type one of the following commands:
    - `:exit`: Exit wim
    - `:save`: Save file
    - `:open`: Open file
    - `theme`: Change theme (gruvbox, dracula)
- `ctrl-s`: Save buffer to file. If a file is opened it simply saves, if the buffer has no filename it prompts the user for a name.
- `ctrl-o`: Shortcut for `:open` command
- `ctrl-n`: Create new empty buffer
- `ctrl-x`: Delete line