# Version 0.8.0 (dev)

**New**

- Visual mode (text highlighting)
- Copy/paste
- Commenting out lines
- Block comments

<br>

# Version 0.7.0

_Released 20.08.24_

**New**

- Split buffers `(ctrl-y)`
- Tabs and switching between them `(ctrl-e)`
- New themes:
  - One-dark
  - Mono

<br>

# Version 0.6.0

_Released 15.07.24_

**New**

- Modal keybinds with vim-like motions
- Find in file
- Help menu `(ctrl-h)`

**What changed**

- Name changed to Rum as the editor will end up being very different from Vim. Why Rum? It's easy to type.

**Small fixes**

- Make builds each file seperately on change
- Better logging methods
- Removed allocations to speed up rendering

<br>

# Version 0.5.0

_Released 09.05.24_

**New**

- Undo
- JSON config files

<br>

# Version 0.4.0

_Released 09.04.24_

**New**

- Synatx support for Fizz [(github.com/jesperkha/Fizz)](github.com/jesperkha/Fizz)

**What changed**

- Complete rewrite of buffer system
- Typing moved to high level api
- Editor can have multiple buffers
- Buffers now handle their own state
- Dynamic buffer rendering
- Removed undo/redo temporarily

**Small fixes**

- Better logging
- Memory fixes

<br>

# Version 0.3.0

_Released 23.02.24_

**New**

- ~~Undo/Redo~~

<br>

# Version 0.2.0

_Released 09.01.24_

**New**

- Better syntax highlighting
- Editor themes
- Config files for themes and syntax
