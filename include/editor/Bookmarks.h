#pragma once

#include <string>
#include <iostream>
#include "editor/Editor.h"
#include "editor/Types.h"
#include "ds/DynamicArray.h"

// Adds a named bookmark at (line, col). Rejects bad names, duplicates, bad ranges.
bool addBookmark(Editor &ed, const std::string &name, int line, int col);

// Adds a bookmark at the current cursor position.
bool addBookmarkAtCursor(Editor &ed, const std::string &name);

// Removes the bookmark with the given name. Returns false if not found.
bool removeBookmark(Editor &ed, const std::string &name);

// Returns the index of the bookmark in ed.bookmarks, or -1 if not found.
int findBookmark(Editor &ed, const std::string &name);

// Jumps the cursor to the named bookmark. Clamps col if the line got shorter.
bool gotoBookmark(Editor &ed, const std::string &name);

// Jumps to the bookmark at the given 0-based index.
bool gotoBookmarkByIndex(Editor &ed, int index);

// Stores the current cursor under "__previous" (replaces if already present).
bool savePreviousPosition(Editor &ed);

// Jumps to "__previous". Returns false if it does not exist.
bool returnToPreviousPosition(Editor &ed);

// Returns the number of bookmarks.
int getBookmarkCount(Editor &ed);

// Prints all non-hidden bookmarks, 1-based numbers.
void listBookmarks(Editor &ed, std::ostream &out);

// Removes all bookmarks.
void clearBookmarks(Editor &ed);

// Shifts bookmarks down when a line is inserted at insertedLine.
void adjustBookmarksOnLineInsert(Editor &ed, int insertedLine);

// Removes/shrinks bookmarks when a line is deleted at deletedLine.
void adjustBookmarksOnLineDelete(Editor &ed, int deletedLine);