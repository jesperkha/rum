# Introduction to using rum

## Editing

By default you will be in EDIT mode. This is the vim-like mode for entering commands and moving around in the editor. Press `i` to enter INSERT mode. In EDIT mode, press `v` to enter VISUAL mode and `V` to enter VISUAL-LINE mode. To go back into EDIT mode press `ctrl-c` or `ESC`. Read more about specific keybinds for each mode with `:help`.

## Commands

When in EDIT mode, press `:` to open the command dialog. Type a command and press enter to run it, or escape to cancel.

## Opening a file

Create a new file with `ctrl-n` or `:n [filepath]`. Use `ctrl-o` (and/or `space-o` when in EDIT mode) to open the file explorer. Navigate with vim keys or arrows. To open a file by path use the `:o [filepath]` command.

## Tabs

Create a new tab with `ctrl-t`. Switch between tabs with `ctrl-e`. Navigate the menu with `j/k` or arrows and select with `space/enter`. Close a tab with `ctrl-w`.

## Split buffers

Enable split buffers with `ctrl-y`, and unsplit the same way. Navigate to the left and right buffer with `ctrl-h` and `ctrl-l`.

## Find

Press `ctrl-f` to find a string in the buffer. The results are incrementally highlighted. Press `enter` to go to the first result. Use `n` and `N` to navigate to next and previous result.
