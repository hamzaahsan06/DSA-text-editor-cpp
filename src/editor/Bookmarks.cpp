#include "editor/Bookmarks.h"
#include "ds/DynamicArray.h"
#include "editor/Editor.h"
#include "editor/Types.h"
#include "editor/Validation.h"

// Design decision: Bookmarks MOVE WITH THE TEXT.
// When a line is inserted above a bookmark, the bookmark's line number increases
// by 1. When a line is deleted above it, it decreases by 1. If the bookmarked
// line itself is deleted, the bookmark is removed. Bookmark changes are NOT
// part of undo/redo and do NOT mark the editor as modified.

// Returns true if name has no leading/trailing whitespace and is non-empty.
static bool isValidName(const std::string& name)
{
    if (isEmptyString(name)) return false;
    unsigned char first = (unsigned char)name[0];
    unsigned char last  = (unsigned char)name[name.length() - 1];
    if (first == ' ' || first == '\t' || first == '\n') return false;
    if (last  == ' ' || last  == '\t' || last  == '\n') return false;
    return true;
}

// Returns true if name starts with the reserved "__" prefix.
static bool isReservedName(const std::string& name)
{
    return name.length() >= 2 && name[0] == '_' && name[1] == '_';
}

// Linear search by name; returns index in ed.bookmarks or -1. O(n).
int findBookmark(Editor& ed, const std::string& name)
{
    int n = ed.bookmarks.size();
    for (int i = 0; i < n; ++i)
    {
        Bookmark b;
        if (!ed.bookmarks.get(i, b)) continue;
        if (b.name == name) return i;
    }
    return -1;
}

// Adds a named bookmark at (line, col) after validation. O(n).
bool addBookmark(Editor& ed, const std::string& name, int line, int col)
{
    if (!isValidName(name)) return false;
    if (isReservedName(name)) return false;
    if (!isValidLine(ed, line)) return false;
    Line* l = ed.getLine(line);
    if (l == nullptr) return false;
    if (!isValidColumn(ed, line, col)) return false;
    if (findBookmark(ed, name) != -1) return false;

    Bookmark b;
    b.name = name;
    b.line = line;
    b.col = col;
    return ed.bookmarks.pushBack(b);
}

// Adds a bookmark at the current cursor position. O(n).
bool addBookmarkAtCursor(Editor& ed, const std::string& name)
{
    return addBookmark(ed, name, ed.cursorLine, ed.cursorCol);
}

// Removes the bookmark with the given name. O(n).
bool removeBookmark(Editor& ed, const std::string& name)
{
    int idx = findBookmark(ed, name);
    if (idx < 0) return false;
    Bookmark dummy;
    return ed.bookmarks.removeAt(idx, dummy);
}

// Jumps the cursor to the named bookmark; clamps col if needed. O(n).
bool gotoBookmark(Editor& ed, const std::string& name)
{
    int idx = findBookmark(ed, name);
    if (idx < 0) return false;

    Bookmark b;
    if (!ed.bookmarks.get(idx, b)) return false;
    if (b.line < 0 || b.line >= ed.getLineCount()) return false;

    Line* l = ed.getLine(b.line);
    if (l == nullptr) return false;

    int col = b.col;
    if (col < 0) col = 0;
    if (col > (int)l->text.length()) col = (int)l->text.length();

    ed.cursorLine = b.line;
    ed.cursorCol = col;
    return true;
}

// Jumps to the bookmark at the given 0-based index. O(n) due to get.
bool gotoBookmarkByIndex(Editor& ed, int index)
{
    if (index < 0) return false;
    int visibleIndex = 0;
    Bookmark b;
    bool found = false;
    for (int i = 0; i < ed.bookmarks.size(); ++i)
    {
        if (!ed.bookmarks.get(i, b) || isReservedName(b.name)) continue;
        if (visibleIndex++ == index)
        {
            found = true;
            break;
        }
    }
    if (!found) return false;
    if (b.line < 0 || b.line >= ed.getLineCount()) return false;

    Line* l = ed.getLine(b.line);
    if (l == nullptr) return false;

    int col = b.col;
    if (col < 0) col = 0;
    if (col > (int)l->text.length()) col = (int)l->text.length();

    ed.cursorLine = b.line;
    ed.cursorCol = col;
    return true;
}

// Stores the current cursor under "__previous" (replace if it exists). O(n).
bool savePreviousPosition(Editor& ed)
{
    // Validate current cursor position.
    if (!isValidLine(ed, ed.cursorLine)) return false;
    Line* l = ed.getLine(ed.cursorLine);
    if (l == nullptr) return false;
    if (!isValidColumn(ed, ed.cursorLine, ed.cursorCol)) return false;

    int idx = findBookmark(ed, "__previous");
    if (idx >= 0)
    {
        Bookmark b;
        if (!ed.bookmarks.get(idx, b)) return false;
        b.line = ed.cursorLine;
        b.col = ed.cursorCol;
        return ed.bookmarks.set(idx, b);
    }

    Bookmark b;
    b.name = "__previous";
    b.line = ed.cursorLine;
    b.col = ed.cursorCol;
    return ed.bookmarks.pushBack(b);
}

// Jumps to "__previous". Returns false if it does not exist. O(n).
bool returnToPreviousPosition(Editor& ed)
{
    return gotoBookmark(ed, "__previous");
}

// Returns the number of bookmarks. O(1).
int getBookmarkCount(Editor& ed)
{
    int visible = 0;
    for (int i = 0; i < ed.bookmarks.size(); ++i)
    {
        Bookmark b;
        if (ed.bookmarks.get(i, b) && !isReservedName(b.name)) ++visible;
    }
    return visible;
}

// Prints all non-hidden bookmarks with 1-based numbering. O(n).
void listBookmarks(Editor& ed, std::ostream& out)
{
    int n = ed.bookmarks.size();
    bool any = false;
    for (int i = 0; i < n; ++i)
    {
        Bookmark b;
        if (!ed.bookmarks.get(i, b)) continue;
        if (isReservedName(b.name)) continue;
        any = true;
        break;
    }

    if (!any)
    {
        out << "No bookmarks saved.\n";
        return;
    }

    int shown = 0;
    for (int i = 0; i < n; ++i)
    {
        Bookmark b;
        if (!ed.bookmarks.get(i, b)) continue;
        if (isReservedName(b.name)) continue;
        ++shown;
        out << shown << ". " << b.name
            << "  (Line " << (b.line + 1)
            << ", Column " << (b.col + 1) << ")\n";
    }
}

// Removes all bookmarks. O(n) in DynamicArray::clear (logical clear).
void clearBookmarks(Editor& ed)
{
    ed.bookmarks.clear();
}

// Increments the line of every bookmark at or after insertedLine. O(n).
void adjustBookmarksOnLineInsert(Editor& ed, int insertedLine)
{
    if (insertedLine < 0) return;
    int n = ed.bookmarks.size();
    for (int i = 0; i < n; ++i)
    {
        Bookmark b;
        if (!ed.bookmarks.get(i, b)) continue;
        if (b.line >= insertedLine)
        {
            b.line += 1;
            ed.bookmarks.set(i, b);
        }
    }
}

// Removes bookmarks on deletedLine; shifts lines above it down by 1. O(n^2).
void adjustBookmarksOnLineDelete(Editor& ed, int deletedLine)
{
    if (deletedLine < 0) return;

    // Loop backwards so removal does not skip elements.
    for (int i = ed.bookmarks.size() - 1; i >= 0; --i)
    {
        Bookmark b;
        if (!ed.bookmarks.get(i, b)) continue;
        if (b.line == deletedLine)
        {
            Bookmark dummy;
            ed.bookmarks.removeAt(i, dummy);
        }
        else if (b.line > deletedLine)
        {
            b.line -= 1;
            ed.bookmarks.set(i, b);
        }
    }
}

// Insertion at a bookmark position moves it with the original text to the right.
void adjustBookmarksOnTextInsert(Editor& ed, int line, int col, int length)
{
    if (line < 0 || col < 0 || length <= 0) return;
    for (int i = 0; i < ed.bookmarks.size(); ++i)
    {
        Bookmark b;
        if (ed.bookmarks.get(i, b) && b.line == line && b.col >= col)
        {
            b.col += length;
            ed.bookmarks.set(i, b);
        }
    }
}

// Bookmarks inside deleted text collapse to the deletion start; later ones shift left.
void adjustBookmarksOnTextDelete(Editor& ed, int line, int col, int length)
{
    if (line < 0 || col < 0 || length <= 0) return;
    const int end = col + length;
    for (int i = 0; i < ed.bookmarks.size(); ++i)
    {
        Bookmark b;
        if (!ed.bookmarks.get(i, b) || b.line != line || b.col < col) continue;
        if (b.col < end) b.col = col;
        else b.col -= length;
        ed.bookmarks.set(i, b);
    }
}

// Positions at or after the split move to the new line, preserving text offset.
void adjustBookmarksOnLineSplit(Editor& ed, int line, int splitCol)
{
    if (line < 0 || splitCol < 0) return;
    for (int i = 0; i < ed.bookmarks.size(); ++i)
    {
        Bookmark b;
        if (ed.bookmarks.get(i, b) && b.line == line && b.col >= splitCol)
        {
            ++b.line;
            b.col -= splitCol;
            ed.bookmarks.set(i, b);
        }
    }
}

// The second line is appended to the first; bookmarks on it shift by the old first-line length.
void adjustBookmarksOnLineMerge(Editor& ed, int firstLine, int firstLineLength)
{
    if (firstLine < 0 || firstLineLength < 0) return;
    for (int i = 0; i < ed.bookmarks.size(); ++i)
    {
        Bookmark b;
        if (!ed.bookmarks.get(i, b)) continue;
        if (b.line == firstLine + 1)
        {
            b.line = firstLine;
            b.col += firstLineLength;
            ed.bookmarks.set(i, b);
        }
        else if (b.line > firstLine + 1)
        {
            --b.line;
            ed.bookmarks.set(i, b);
        }
    }
}
