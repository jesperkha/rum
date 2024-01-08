#pragma once

// Writes characters to buffer at cursor pos. Does not filter out non-writable characters.
void BufferWrite(char *source, int length);

// Deletes the caharcter before the cursor position. Deletes line if cursor is at beginning.
void BufferDeleteChar();

// Inserts new line at row. If row is -1 line is appended to end of file.
void BufferInsertLine(int row);

// Deletes line at row and move all lines below upwards.
void BufferDeleteLine(int row);

// Copies and removes all characters behind the cursor position,
// then pastes them at the end of the line below.
void BufferSplitLineDown(int row);

// Moves line content from row to end of line above.
void BufferSplitLineUp(int row);

// Scrolls buffer vertically by delta y.
void BufferScroll(int dy);

// Shorthands for scrolling up and down by one. Moves cursor too.
void BufferScrollDown();
void BufferScrollUp();
