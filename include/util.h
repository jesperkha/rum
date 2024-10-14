#pragma once

#define clamp(MIN, MAX, v) (max(min((v), (MAX)), (MIN)))
#define capValue(v, MAX) \
    {                    \
        if ((v) > (MAX)) \
            (v) = (MAX); \
    }

// Splits by c. Returns split+1, puts split ptr to splitBegin and its length to length.
// Returns NULL on last split, splitBegin is NULL the iteration after.
char *StrSplitNext(char *s, char c, char **splitBegin, int *length);
int StrCount(char *s, char c);
// Caps width of string by replacing spaces with newlines. Source is modified.
void StrCapWidth(char *source, int maxW);
// Gets filename, including extension, from filepath
void StrFilename(char *dest, char *src);
// Gets the file extension, excluding the peroid.
void StrFileExtension(char *dest, char *src);
// Returns pointer to first character in first instance of substr in buf. NULL if none is found.
char *StrMemStr(char *buf, char *substr, size_t size);
// Modifies path if add ~ at beginning if path is in home directory.
// Returns pointer to beginning of short path, which is withing path.
// If the path is not withing home, the returned pointer is just path.
char *StrGetShortPath(char *path);
void StrReplace(char *s, char find, char replace);
// Converts n to a readable format: 18200 -> 18K etc. Unsafe.
void StrNumberToReadable(unsigned long long n, char *dest);
// Returns true if c is a printable ascii character
bool isChar(char c);

// Used to store text before rendering.
typedef struct CharBuf
{
    char *buffer;
    char *pos;
    int lineLength;
} CharBuf;

// Returns empty CharBuf mapped to input buffer.
CharBuf CbNew(char *buffer);
// Resets buffer to starting state. Does not memclear the internal buffer.
void CbReset(CharBuf *buf);
void CbAppend(CharBuf *buf, char *src, int length);
void CbRepeat(CharBuf *buf, char c, int count);
// Fills remaining line with space characters based on editor width.
void CbNextLine(CharBuf *buf);
// Adds background and foreground color to buffer.
void CbColor(CharBuf *buf, char *bg, char *fg);
void CbColorWord(CharBuf *cb, char *fg, char *word, int wordlen);
void CbBg(CharBuf *buf, char *bg);
void CbFg(CharBuf *buf, char *fg);
// Adds COL_RESET to buffer
void CbColorReset(CharBuf *buf);
// Prints buffer at x, y with accumulated length only.
void CbRender(CharBuf *buf, int x, int y);
// Returns total byte length written to buffer
int CbLength(CharBuf *cb);

typedef struct StrArray
{
    int length;
    int cap;
    char *ptr;
} StrArray;

StrArray StrArrayNew(int size);
int StrArraySet(StrArray *a, char *source, int length);
char *StrArrayGet(StrArray *a, int idx);
void StrArrayFree(StrArray *a);

void *MemAlloc(int size);
void *MemZeroAlloc(int size);
void *MemRealloc(void *ptr, int newSize);
void MemFree(void *ptr);

// Read file realitive to cwd. Writes to size. Returns null on failure. Free content pointer.
char *IoReadFile(const char *filepath, int *size);
// Truncates file or creates new one if it doesnt exist. Returns true on success.
bool IoWriteFile(const char *filepath, char *data, int size);