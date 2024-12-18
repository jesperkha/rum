#pragma once

Buffer *BufferNew();
void BufferFree(Buffer *b);

// Writes characters to buffer at cursor position.
void BufferWrite(Buffer *buf, char *source, int length);
void BufferWriteEx(Buffer *buf, int row, int col, char *source, int length);
// Writes to buffer at row/col. Replaces any characters that are already there.
void BufferOverWrite(Buffer *b, char *source, int length);
void BufferOverWriteEx(Buffer *b, int row, int col, char *source, int length);
// Deletes backwards from cursor pos. Stops at empty line, does not remove newline.
void BufferDelete(Buffer *buf, int count);
void BufferDeleteEx(Buffer *buf, int row, int col, int count);
// Inserts new line at row. If row is -1 line is appended to end of file. Returns new line.
Line *BufferInsertLine(Buffer *buf, int row);
Line *BufferInsertLineEx(Buffer *b, int row, char *text, int textLen);
// Deletes line at row and move all lines below upwards.
void BufferDeleteLine(Buffer *buf, int row);
// Copies and removes all characters behind the cursor position,
// then pastes them at the end of the line below.
void BufferMoveTextDown(Buffer *buf);
void BufferMoveTextDownEx(Buffer *buf, int row, int col);
// Moves line content from row to end of line above.
int BufferMoveTextUp(Buffer *buf);
int BufferMoveTextUpEx(Buffer *buf, int row, int col);
// Sets buffer scroll based on real cursor position
void BufferScroll(Buffer *buf);
// Returns number of spaces before the cursor
int BufferGetPrefixedSpaces(Buffer *buf);
// Draws buffer to fill entire window
void BufferRenderFull(Buffer *b);
// Draws two buffers of same size split horizontally
void BufferRenderSplit(Buffer *a, Buffer *b);
// Loads file contents into a new Buffer and returns it. Returns NULL on failure.
Buffer *BufferLoadFile(char *filepath, char *buf, int size);
// Saves buffer contents to file. Returns true on success.
bool BufferSaveFile(Buffer *b);
// Scrolls buffer such that cursor is at center
void BufferCenterView(Buffer *b);
// Assigns ordered highlight points to from and to
void BufferOrderHighlightPoints(Buffer *b, CursorPos *from, CursorPos *to);
// Returns the text hihglighted in visual mode
char *BufferGetMarkedText(Buffer *b);
// Returns path for given line if it is a filepath in the explorer or a string path
char *BufferGetLinePath(Buffer *b, Line *line);
// Sets current search word in buffer. NULL is accepted.
void BufferSetSearchWord(Buffer *b, char *search, int length);
// Marks part of a line with yellow background for search.
void BufferMarkLine(Buffer *b, int row, int col, int length);
// Unmarks all lines after a search.
void BufferUnmarkAll(Buffer *b);
// Sets filename for buffer and marks it as an open file
void BufferSetFilename(Buffer *b, char *filepath);
// Sets filetype for buffer. Only affects syntax hl. Returns true if set successfully.
bool BufferSetFileType(Buffer *b, const char *extension);

// Sets cursor position in buffer space, scrolls if necessary. keepX is true when the cursor
// should keep the current max width when moving vertically, only really used with CursorMove.
void CursorSetPos(Buffer *buf, int x, int y, bool keepX);
// Moves cursor by x,y. Updates buffer scroll.
void CursorMove(Buffer *buf, int x, int y);
// Sets the cursor pos without additional stuff happening. The editor position is
// not updated so cursor returns to previous position when render is called.
void CursorTempPos(int x, int y);
void CursorHide();
void CursorShow();
void CursorUpdatePos();