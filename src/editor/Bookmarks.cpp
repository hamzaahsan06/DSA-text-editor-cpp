#include "editor/Bookmarks.h"
#include "ds/DynamicArray.h"
#include "editor/Editor.h"
#include "editor/Types.h"

static const char *PREVIOUS_SLOT_NAME = "__previous";

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

static bool isReservedName(const std::string &name)
{
    if (name.length() < 2)
    {
        return false;
    }
    return name[0] == '_' && name[1] == '_';
}

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

bool addBookmarkAtCursor(Editor &ed, const std::string &name)
{
    return addBookmark(ed, name, ed.cursorLine, ed.cursorCol);
}

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

// Jumps to the visible (non-reserved) bookmark at the given 0-based index.
bool gotoBookmarkByIndex(Editor &ed, int index)
{
    if (index < 0)
    {
        return false;
    }
    int visibleIndex = 0;
    int total = ed.bookmarks.size();
    for (int i = 0; i < total; ++i)
    {
        Bookmark target;
        if (!ed.bookmarks.get(i, target))
        {
            continue;
        }
        if (isReservedName(target.name))
        {
            continue;
        }
        if (visibleIndex == index)
        {
            return moveCursorTo(ed, target.line, target.col);
        }
        ++visibleIndex;
    }
    return false;
}

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

bool returnToPreviousPosition(Editor &ed)
{
    return gotoBookmark(ed, PREVIOUS_SLOT_NAME);
}

// Returns the number of visible (non-reserved) bookmarks.
int getBookmarkCount(Editor &ed)
{
    int visible = 0;
    int total = ed.bookmarks.size();
    for (int i = 0; i < total; ++i)
    {
        Bookmark current;
        if (ed.bookmarks.get(i, current) && !isReservedName(current.name))
        {
            ++visible;
        }
    }
    return visible;
}

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

void clearBookmarks(Editor &ed)
{
    ed.bookmarks.clear();
}

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