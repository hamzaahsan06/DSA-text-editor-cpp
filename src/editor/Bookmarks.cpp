#include "editor/Bookmarks.h"
#include "ds/DynamicArray.h"
#include "editor/Editor.h"
#include "editor/Types.h"

// Design decision: bookmarks move with the text.
// Inserting a line above a bookmark shifts its line number up by one;
// deleting a line above it shifts it down. If the bookmarked line itself
// is deleted, the bookmark is removed. Bookmarks are not undoable and
// do not mark the editor as modified.

// Reserved name for savePreviousPosition / returnToPreviousPosition.
static const char *PREVIOUS_SLOT_NAME = "__previous";

// Returns true if name is non-empty and contains no whitespace.
static bool isValidName(const std::string &name)
{
    if (name.empty())
    {
        return false;
    }
    for (int i = 0; i < (int)name.length(); ++i)
    {
        char c = name[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        {
            return false;
        }
    }
    return true;
}

// Returns true if name uses the reserved "__" prefix.
static bool isReservedName(const std::string &name)
{
    if (name.length() < 2)
    {
        return false;
    }
    return name[0] == '_' && name[1] == '_';
}

// Returns the text length of the given line, or -1 if the index is invalid.
static int lineLengthAt(Editor &ed, int lineIndex)
{
    if (lineIndex < 0 || lineIndex >= ed.getLineCount())
    {
        return -1;
    }
    Line *line = ed.getLine(lineIndex);
    if (line == nullptr)
    {
        return -1;
    }
    return (int)line->text.length();
}

// Moves the cursor to (line, col), clamping col to the line length.
static bool moveCursorTo(Editor &ed, int line, int col)
{
    int length = lineLengthAt(ed, line);
    if (length < 0)
    {
        return false;
    }
    if (col < 0)
    {
        col = 0;
    }
    if (col > length)
    {
        col = length;
    }
    ed.cursorLine = line;
    ed.cursorCol = col;
    return true;
}

// Linear search by name; returns index in ed.bookmarks or -1.
int findBookmark(Editor &ed, const std::string &name)
{
    int total = ed.bookmarks.size();
    for (int i = 0; i < total; ++i)
    {
        Bookmark current;
        if (!ed.bookmarks.get(i, current))
        {
            continue;
        }
        if (current.name == name)
        {
            return i;
        }
    }
    return -1;
}

// Validates the name, position, and duplicate; then appends the bookmark.
bool addBookmark(Editor &ed, const std::string &name, int line, int col)
{
    if (!isValidName(name))
    {
        return false;
    }
    if (isReservedName(name))
    {
        return false;
    }
    int length = lineLengthAt(ed, line);
    if (length < 0)
    {
        return false;
    }
    if (col < 0 || col > length)
    {
        return false;
    }
    if (findBookmark(ed, name) != -1)
    {
        return false;
    }

    Bookmark fresh;
    fresh.name = name;
    fresh.line = line;
    fresh.col = col;
    return ed.bookmarks.pushBack(fresh);
}

// Adds a bookmark at the current cursor position.
bool addBookmarkAtCursor(Editor &ed, const std::string &name)
{
    return addBookmark(ed, name, ed.cursorLine, ed.cursorCol);
}

// Removes the bookmark with the given name.
bool removeBookmark(Editor &ed, const std::string &name)
{
    int index = findBookmark(ed, name);
    if (index < 0)
    {
        return false;
    }
    Bookmark removed;
    return ed.bookmarks.removeAt(index, removed);
}

// Jumps the cursor to the named bookmark, clamping col if needed.
bool gotoBookmark(Editor &ed, const std::string &name)
{
    int index = findBookmark(ed, name);
    if (index < 0)
    {
        return false;
    }
    Bookmark target;
    if (!ed.bookmarks.get(index, target))
    {
        return false;
    }
    return moveCursorTo(ed, target.line, target.col);
}

// Jumps to the bookmark at the given 0-based index.
bool gotoBookmarkByIndex(Editor &ed, int index)
{
    if (index < 0 || index >= ed.bookmarks.size())
    {
        return false;
    }
    Bookmark target;
    if (!ed.bookmarks.get(index, target))
    {
        return false;
    }
    return moveCursorTo(ed, target.line, target.col);
}

// Stores the current cursor under the reserved previous-position slot.
bool savePreviousPosition(Editor &ed)
{
    int length = lineLengthAt(ed, ed.cursorLine);
    if (length < 0)
    {
        return false;
    }
    if (ed.cursorCol < 0 || ed.cursorCol > length)
    {
        return false;
    }

    int existingIndex = findBookmark(ed, PREVIOUS_SLOT_NAME);
    if (existingIndex >= 0)
    {
        Bookmark updated;
        if (!ed.bookmarks.get(existingIndex, updated))
        {
            return false;
        }
        updated.line = ed.cursorLine;
        updated.col = ed.cursorCol;
        return ed.bookmarks.set(existingIndex, updated);
    }

    Bookmark fresh;
    fresh.name = PREVIOUS_SLOT_NAME;
    fresh.line = ed.cursorLine;
    fresh.col = ed.cursorCol;
    return ed.bookmarks.pushBack(fresh);
}

// Jumps to the reserved previous-position slot.
bool returnToPreviousPosition(Editor &ed)
{
    return gotoBookmark(ed, PREVIOUS_SLOT_NAME);
}

// Returns the number of bookmarks.
int getBookmarkCount(Editor &ed)
{
    return ed.bookmarks.size();
}

// Prints all visible bookmarks with 1-based numbering, or a "none" message.
void listBookmarks(Editor &ed, std::ostream &out)
{
    int total = ed.bookmarks.size();

    bool anyVisible = false;
    for (int i = 0; i < total; ++i)
    {
        Bookmark current;
        if (!ed.bookmarks.get(i, current))
        {
            continue;
        }
        if (!isReservedName(current.name))
        {
            anyVisible = true;
            break;
        }
    }

    if (!anyVisible)
    {
        out << "No bookmarks saved.\n";
        return;
    }

    int shown = 0;
    for (int i = 0; i < total; ++i)
    {
        Bookmark current;
        if (!ed.bookmarks.get(i, current))
        {
            continue;
        }
        if (isReservedName(current.name))
        {
            continue;
        }
        ++shown;
        out << shown << ". " << current.name
            << "  (Line " << (current.line + 1)
            << ", Column " << (current.col + 1) << ")\n";
    }
}

// Removes all bookmarks, keeping the underlying buffer for reuse.
void clearBookmarks(Editor &ed)
{
    ed.bookmarks.clear();
}

// Shifts bookmark lines up by one when a line is inserted at or above them.
void adjustBookmarksOnLineInsert(Editor &ed, int insertedLine)
{
    if (insertedLine < 0)
    {
        return;
    }
    int total = ed.bookmarks.size();
    for (int i = 0; i < total; ++i)
    {
        Bookmark current;
        if (!ed.bookmarks.get(i, current))
        {
            continue;
        }
        if (current.line >= insertedLine)
        {
            current.line += 1;
            ed.bookmarks.set(i, current);
        }
    }
}

// Drops bookmarks on the deleted line and shifts those below it up by one.
// Loop runs backwards so removals do not skip elements.
void adjustBookmarksOnLineDelete(Editor &ed, int deletedLine)
{
    if (deletedLine < 0)
    {
        return;
    }
    for (int i = ed.bookmarks.size() - 1; i >= 0; --i)
    {
        Bookmark current;
        if (!ed.bookmarks.get(i, current))
        {
            continue;
        }
        if (current.line == deletedLine)
        {
            Bookmark removed;
            ed.bookmarks.removeAt(i, removed);
        }
        else if (current.line > deletedLine)
        {
            current.line -= 1;
            ed.bookmarks.set(i, current);
        }
    }
}

// Adjust bookmarks when a line is split at (line, col).
// Bookmarks on `line` at col >= split move down and shift left.
// Bookmarks below `line` move down by one.
void adjustBookmarksOnSplit(Editor &ed, int line, int col)
{
    if (line < 0 || col < 0)
    {
        return;
    }
    int total = ed.bookmarks.size();
    for (int i = 0; i < total; ++i)
    {
        Bookmark current;
        if (!ed.bookmarks.get(i, current))
        {
            continue;
        }
        if (current.line == line && current.col >= col)
        {
            current.line += 1;
            current.col -= col;
            ed.bookmarks.set(i, current);
        }
        else if (current.line > line)
        {
            current.line += 1;
            ed.bookmarks.set(i, current);
        }
    }
}

// Adjust bookmarks when the line below `line` is merged back into `line`.
// Bookmarks on line+1 move up and shift right. Bookmarks below move up.
void adjustBookmarksOnJoin(Editor &ed, int line, int col)
{
    if (line < 0 || col < 0)
    {
        return;
    }
    int total = ed.bookmarks.size();
    for (int i = 0; i < total; ++i)
    {
        Bookmark current;
        if (!ed.bookmarks.get(i, current))
        {
            continue;
        }
        if (current.line == line + 1)
        {
            current.line -= 1;
            current.col += col;
            ed.bookmarks.set(i, current);
        }
        else if (current.line > line + 1)
        {
            current.line -= 1;
            ed.bookmarks.set(i, current);
        }
    }
}

// Copies every bookmark whose line is in [startLine, endLine] to `out`.
void collectBookmarksInLines(Editor &ed, int startLine, int endLine, DynamicArray<Bookmark> &out)
{
    if (startLine < 0 || endLine < startLine)
    {
        return;
    }
    int total = ed.bookmarks.size();
    for (int i = 0; i < total; ++i)
    {
        Bookmark current;
        if (!ed.bookmarks.get(i, current))
        {
            continue;
        }
        if (current.line >= startLine && current.line <= endLine)
        {
            out.pushBack(current);
        }
    }
}

// Restores saved bookmarks. Skips existing names and non-existent lines.
// Clamps columns past the end of the line.
void restoreBookmarks(Editor &ed, const DynamicArray<Bookmark> &saved)
{
    int total = saved.size();
    for (int i = 0; i < total; ++i)
    {
        Bookmark current;
        if (!saved.get(i, current))
        {
            continue;
        }
        // Skip if a bookmark with this name already exists.
        if (findBookmark(ed, current.name) != -1)
        {
            continue;
        }
        // Skip if the line does not exist.
        if (current.line < 0 || current.line >= ed.getLineCount())
        {
            continue;
        }
        // Clamp column to the line length.
        Line *line = ed.getLine(current.line);
        if (line != nullptr && current.col > (int)line->text.length())
        {
            current.col = (int)line->text.length();
        }
        if (current.col < 0)
        {
            current.col = 0;
        }
        // Append directly (do not use addBookmark, to allow "__previous").
        ed.bookmarks.pushBack(current);
    }
}