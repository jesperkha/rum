<br />
<div align="center">
  <img src=".github/logo.svg" alt="Logo" width="180">

  <p align="center">
    <b>Minimal editor for the Windows console</b>
    <br>
    <a href="https://github.com/jesperkha/rum/releases/latest"><strong>Latest release »</strong></a>
    <br />
    <br />
  </p>
</div>

<div align="center">
<img src=".github/screenshot.png" alt="Screenshot" width="80%">

<i>Rum editing its own source code</i>

</div>

## About

Rum is a fast and minimal editor that supports syntax highlighting, search, split buffers, tabs and much more! It is specifically made for the windows terminal using the win32 console API. It has no other dependencies than libc and win32, making it very lightweight (~70kb) and easy to build! See [roadmap.md](roadmap.md) and [changelog.md](changelog.md) for progress on development, and [the tutorial](tutorial.md) to get started.

### Key features

- Easy to use and install
- Super lightweight
- No dependencies, runs out of the box
- Vim-like keybindings
- Sane defaults
- Split buffers and tabs
- Syntax highlighting and themes
- File explorer

## Installation

### Just install rum please

[Download and run the installer.](https://github.com/jesperkha/rum/releases/latest) This will put rum in your `Program Files` folder and add it to the PATH environment variable. Then run it from anywhere with Command Prompt or Windows Terminal!

### Or build from source

This requires mingw and gcc. Note that, when moving the executable to another location, you need to copy the `config` directory along with it.

```
git clone https://github.com/jesperkha/rum.git
cd rum
gcc build.c -o build.exe && ./build.exe
```

## Windows Only

rum is only for Windows at the moment. This will not change any time soon. rum was initially made to be a 100% Windows compatible terminal text editor with no dependencies. For this reason, rum is not designed to be cross platform, and there is little to no abstraction over OS specific code. Linux user can enjoy Vim for now...

## Documentation and Help

Using the command `:help` will display all available commands in rum. You can also read [the tutorial](tutorial.md).
