<br />
<div align="center">
  <img src=".github/logo.svg" alt="Logo" width="180">

  <h4 align="center">Simple terminal editor for the windows console.</h4>

  <p align="center">
    Currently supports basic text editing, syntax
    highlighting, typing features such <br> as parenthesis matching and dynamic tabs, and config files for syntax and themes.
    <br />
    <a href="https://github.com/jesperkha/rum/releases/tag/v0.5.0"><strong>Latest release »</strong></a>
    <br />
    <br />
  </p>
</div>

## About

Rum is a terminal editor made for the Windows terminal, using the win32 console API. It has no other dependencies than libc and win32, making it very lightweight (~50kb). See [roadmap.md](roadmap.md) and [changelog.md](changelog.md) for progress on development.

Build with make. Usage: `rum [filename]`

**Note:** When moving the executable to another location, make sure you copy the `config` directory along with it.

## Screenshots

<div align="center">
<img src=".github/screenshot.png" alt="Screenshot" width="90%">

<a href="https://github.com/jesperkha/rum/blob/main/.github/demo.gif">Demo gif</a>

</div>

## Controls

- `ctrl-q`: Exit rum. Pressing the escape key will do the same.
- `ctrl-s`: Save file
- `ctrl-o`: Open file
- `ctrl-n`: Create new file
- `ctrl-x`: Delete line
- `ctrl-u`: Undo
- `ctrl-c`: Command line (exit with ESC)
  - `:exit`: Exit rum
  - `:save`: Save file
  - `:open [filename]`: Open file
  - `:theme [theme]`: Change theme (gruvbox, dracula)
