#pragma once

#include <string>
#include <iostream>
#include "editor/Editor.h"
#include "editor/Types.h"
#include "ds/DynamicArray.h"

// Bookmarks: named cursor positions stored in a DynamicArray.
// Bookmarks move with the text (they track content, not fixed numbers),
// and are NOT part of undo/redo and do NOT mark the editor as modified.

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

// Stores the current cursor under the reserved previous-position slot.
bool savePreviousPosition(Editor &ed);

// Jumps to the previous-position slot. Returns false if it does not exist.
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

// NEW: Adjust bookmarks when a line is split at (line, col).
// Bookmarks on `line` at col >= split move to line+1 with col reduced.
// Bookmarks on lines below `line` move down by one.
void adjustBookmarksOnSplit(Editor &ed, int line, int col);

// NEW: Adjust bookmarks when the line below `line` is merged back into `line`
// at column `col`. This is the exact inverse of adjustBookmarksOnSplit.
void adjustBookmarksOnJoin(Editor &ed, int line, int col);

// NEW: Copies (does not remove) every bookmark whose line is in [startLine, endLine]
// into `out`. Includes the hidden "__previous" slot.
void collectBookmarksInLines(Editor &ed, int startLine, int endLine, DynamicArray<Bookmark> &out);

// NEW: Restores saved bookmarks. Skips names that already exist, and skips
// bookmarks whose line no longer exists. Clamps columns that are too long.
void restoreBookmarks(Editor &ed, const DynamicArray<Bookmark> &saved);