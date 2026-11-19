// g++ -std=c++17 -Wall -Wextra -Iinclude -Iinclude/editor tests/editor/test_Bookmarks.cpp src/editor/Bookmarks.cpp -o build/test_Bookmarks ; if ($?) { ./build/test_Bookmarks }

#include <iostream>
#include <sstream>
#include <string>
#include "editor/Bookmarks.h"
#include "editor/Editor.h"
#include "editor/Types.h"
#include "../ds/check.h"

// Builds an editor with the given lines. Editor is non-copyable, so fill in place.
static void makeEditorWith(const std::string *lines, int n, Editor &ed)
{
    ed.reset();
    if (n <= 0)
        return;
    Line *first = ed.getLine(0);
    if (first != nullptr)
        first->text = lines[0];
    for (int i = 1; i < n; ++i)
    {
        Line l;
        l.text = lines[i];
        ed.document.insertAtTail(l);
    }
}

// ---------- addBookmark ----------

// Adds a bookmark and findBookmark returns its index.
void test_add_and_find_bookmark()
{
    const std::string lines[] = {"hello", "world"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    CHECK(addBookmark(ed, "start", 0, 0) == true);
    CHECK(getBookmarkCount(ed) == 1);
    CHECK(findBookmark(ed, "start") == 0);
    CHECK(addBookmark(ed, "end", 1, 5) == true);
    CHECK(getBookmarkCount(ed) == 2);
    CHECK(findBookmark(ed, "end") == 1);
}

// Rejects an empty name.
void test_add_empty_name_rejected()
{
    const std::string lines[] = {"hello"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(addBookmark(ed, "", 0, 0) == false);
    CHECK(getBookmarkCount(ed) == 0);
}

// Rejects a name containing whitespace anywhere (new strict rule).
void test_add_whitespace_name_rejected()
{
    const std::string lines[] = {"hello"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(addBookmark(ed, " start", 0, 0) == false);  // leading
    CHECK(addBookmark(ed, "start ", 0, 0) == false);  // trailing
    CHECK(addBookmark(ed, "mid dle", 0, 0) == false); // middle
    CHECK(addBookmark(ed, "tab\there", 0, 0) == false);
    CHECK(addBookmark(ed, "new\nline", 0, 0) == false);
    CHECK(addBookmark(ed, "cr\rhere", 0, 0) == false);
    CHECK(getBookmarkCount(ed) == 0);
}

// Names with no whitespace (underscores, digits, punctuation) are accepted.
void test_add_name_without_whitespace_accepted()
{
    const std::string lines[] = {"hello"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(addBookmark(ed, "start_point", 0, 0) == true);
    CHECK(addBookmark(ed, "a1", 0, 0) == true);
    CHECK(addBookmark(ed, "x.y-z", 0, 0) == true);
    CHECK(getBookmarkCount(ed) == 3);
}

// Rejects a reserved "__" prefixed name.
void test_add_reserved_name_rejected()
{
    const std::string lines[] = {"hello"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(addBookmark(ed, "__previous", 0, 0) == false);
    CHECK(addBookmark(ed, "__internal", 0, 0) == false);
    CHECK(addBookmark(ed, "__", 0, 0) == false);
    CHECK(getBookmarkCount(ed) == 0);
}

// Rejects a duplicate name.
void test_add_duplicate_name_rejected()
{
    const std::string lines[] = {"hello"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(addBookmark(ed, "spot", 0, 0) == true);
    CHECK(addBookmark(ed, "spot", 0, 1) == false);
    CHECK(getBookmarkCount(ed) == 1);
    int idx = findBookmark(ed, "spot");
    CHECK(idx == 0);
    Bookmark b;
    CHECK(ed.bookmarks.get(idx, b) == true);
    CHECK(b.line == 0 && b.col == 0);
}

// Rejects out-of-range line.
void test_add_bad_line_rejected()
{
    const std::string lines[] = {"hello"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(addBookmark(ed, "neg", -1, 0) == false);
    CHECK(addBookmark(ed, "past", 1, 0) == false);
    CHECK(addBookmark(ed, "far", 100, 0) == false);
    CHECK(getBookmarkCount(ed) == 0);
}

// Rejects negative col and col > line length; accepts col == length.
void test_add_bad_col_rejected_and_end_of_line_accepted()
{
    const std::string lines[] = {"abcde"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(addBookmark(ed, "negcol", 0, -1) == false);
    CHECK(addBookmark(ed, "pastend", 0, 6) == false);
    CHECK(addBookmark(ed, "eol", 0, 5) == true); // col == length allowed
    CHECK(findBookmark(ed, "eol") == 0);
}

// Adds a bookmark at the current cursor.
void test_add_at_cursor_uses_editor_cursor()
{
    const std::string lines[] = {"first", "second"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    ed.cursorLine = 1;
    ed.cursorCol = 3;
    CHECK(addBookmarkAtCursor(ed, "cur") == true);
    int idx = findBookmark(ed, "cur");
    CHECK(idx == 0);
    Bookmark b;
    CHECK(ed.bookmarks.get(idx, b) == true);
    CHECK(b.line == 1);
    CHECK(b.col == 3);
}

// addBookmarkAtCursor rejects an invalid cursor position.
void test_add_at_cursor_rejects_bad_cursor()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    ed.cursorLine = 0;
    ed.cursorCol = 99;
    CHECK(addBookmarkAtCursor(ed, "bad") == false);
    CHECK(getBookmarkCount(ed) == 0);
}

// ---------- findBookmark / removeBookmark ----------

// findBookmark returns -1 when the name is unknown.
void test_find_unknown_returns_minus_one()
{
    const std::string lines[] = {"a"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(findBookmark(ed, "nothing") == -1);
    CHECK(findBookmark(ed, "") == -1);
}

// removeBookmark removes an existing bookmark and preserves order.
void test_remove_existing()
{
    const std::string lines[] = {"a", "b", "c"};
    Editor ed;
    makeEditorWith(lines, 3, ed);
    addBookmark(ed, "x", 0, 0);
    addBookmark(ed, "y", 1, 0);
    addBookmark(ed, "z", 2, 0);
    CHECK(getBookmarkCount(ed) == 3);
    CHECK(removeBookmark(ed, "y") == true);
    CHECK(getBookmarkCount(ed) == 2);
    CHECK(findBookmark(ed, "y") == -1);
    CHECK(findBookmark(ed, "x") == 0);
    CHECK(findBookmark(ed, "z") == 1);
}

// removeBookmark returns false for an unknown name.
void test_remove_unknown()
{
    const std::string lines[] = {"a"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(removeBookmark(ed, "nope") == false);
    CHECK(removeBookmark(ed, "") == false);
}

// ---------- gotoBookmark / gotoBookmarkByIndex ----------

// gotoBookmark sets the cursor to the saved position.
void test_goto_sets_cursor()
{
    const std::string lines[] = {"hello", "world"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    addBookmark(ed, "w", 1, 3);
    ed.cursorLine = 0;
    ed.cursorCol = 0;
    CHECK(gotoBookmark(ed, "w") == true);
    CHECK(ed.cursorLine == 1);
    CHECK(ed.cursorCol == 3);
}

// gotoBookmark clamps col if the line shrank.
void test_goto_clamps_col_when_line_shrank()
{
    const std::string lines[] = {"longline"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    addBookmark(ed, "tip", 0, 7);
    Line *l = ed.getLine(0);
    l->text = "lo";
    CHECK(gotoBookmark(ed, "tip") == true);
    CHECK(ed.cursorLine == 0);
    CHECK(ed.cursorCol == 2);
}

// gotoBookmark returns false for unknown name and leaves cursor alone.
void test_goto_unknown_leaves_cursor()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    ed.cursorLine = 0;
    ed.cursorCol = 1;
    CHECK(gotoBookmark(ed, "missing") == false);
    CHECK(ed.cursorLine == 0);
    CHECK(ed.cursorCol == 1);
}

// gotoBookmark returns false when the saved line no longer exists.
void test_goto_returns_false_when_line_gone()
{
    const std::string lines[] = {"a", "b", "c"};
    Editor ed;
    makeEditorWith(lines, 3, ed);
    addBookmark(ed, "gone", 2, 0);
    Line dummy;
    ed.document.removeAtPosition(2, dummy);
    CHECK(gotoBookmark(ed, "gone") == false);
}

// gotoBookmarkByIndex sets the cursor.
void test_goto_by_index_sets_cursor()
{
    const std::string lines[] = {"hello", "world"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    addBookmark(ed, "a", 0, 1);
    addBookmark(ed, "b", 1, 4);
    ed.cursorLine = 0;
    ed.cursorCol = 0;
    CHECK(gotoBookmarkByIndex(ed, 1) == true);
    CHECK(ed.cursorLine == 1);
    CHECK(ed.cursorCol == 4);
}

// gotoBookmarkByIndex rejects bad indexes.
void test_goto_by_index_bad_index()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    addBookmark(ed, "only", 0, 0);
    CHECK(gotoBookmarkByIndex(ed, -1) == false);
    CHECK(gotoBookmarkByIndex(ed, 1) == false);
    CHECK(gotoBookmarkByIndex(ed, 99) == false);
}

// gotoBookmarkByIndex clamps col if the line shrank.
void test_goto_by_index_clamps_col()
{
    const std::string lines[] = {"longline"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    addBookmark(ed, "tip", 0, 7);
    Line *l = ed.getLine(0);
    l->text = "lo";
    CHECK(gotoBookmarkByIndex(ed, 0) == true);
    CHECK(ed.cursorCol == 2);
}

// ---------- savePreviousPosition / returnToPreviousPosition ----------

// savePreviousPosition stores the cursor under "__previous".
void test_save_previous_stores_position()
{
    const std::string lines[] = {"abc", "def"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    ed.cursorLine = 1;
    ed.cursorCol = 2;
    CHECK(savePreviousPosition(ed) == true);
    int idx = findBookmark(ed, "__previous");
    CHECK(idx >= 0);
    Bookmark b;
    CHECK(ed.bookmarks.get(idx, b) == true);
    CHECK(b.line == 1);
    CHECK(b.col == 2);
}

// savePreviousPosition updates the existing "__previous" bookmark.
void test_save_previous_updates_existing()
{
    const std::string lines[] = {"abc", "def"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    ed.cursorLine = 0;
    ed.cursorCol = 1;
    CHECK(savePreviousPosition(ed) == true);
    ed.cursorLine = 1;
    ed.cursorCol = 2;
    CHECK(savePreviousPosition(ed) == true);
    CHECK(getBookmarkCount(ed) == 1);
    Bookmark b;
    int idx = findBookmark(ed, "__previous");
    CHECK(ed.bookmarks.get(idx, b) == true);
    CHECK(b.line == 1);
    CHECK(b.col == 2);
}

// savePreviousPosition rejects an invalid cursor without crashing.
void test_save_previous_rejects_bad_cursor()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    ed.cursorCol = 99;
    CHECK(savePreviousPosition(ed) == false);
    CHECK(getBookmarkCount(ed) == 0);
}

// returnToPreviousPosition jumps to "__previous".
void test_return_previous_jumps()
{
    const std::string lines[] = {"abc", "def"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    ed.cursorLine = 0;
    ed.cursorCol = 1;
    savePreviousPosition(ed);
    ed.cursorLine = 1;
    ed.cursorCol = 2;
    CHECK(returnToPreviousPosition(ed) == true);
    CHECK(ed.cursorLine == 0);
    CHECK(ed.cursorCol == 1);
}

// returnToPreviousPosition returns false when nothing was saved.
void test_return_previous_missing()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    ed.cursorLine = 0;
    ed.cursorCol = 0;
    CHECK(returnToPreviousPosition(ed) == false);
    CHECK(ed.cursorLine == 0);
    CHECK(ed.cursorCol == 0);
}

// ---------- getBookmarkCount / listBookmarks / clearBookmarks ----------

// getBookmarkCount matches the array size, including hidden ones.
void test_count_includes_hidden()
{
    const std::string lines[] = {"a", "b"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    CHECK(getBookmarkCount(ed) == 0);
    addBookmark(ed, "x", 0, 0);
    addBookmark(ed, "y", 1, 0);
    CHECK(getBookmarkCount(ed) == 2);
    ed.cursorLine = 0;
    ed.cursorCol = 0;
    savePreviousPosition(ed);
    CHECK(getBookmarkCount(ed) == 3);
}

// listBookmarks prints "No bookmarks saved." when empty.
void test_list_empty()
{
    const std::string lines[] = {"a"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    std::ostringstream out;
    listBookmarks(ed, out);
    CHECK(out.str() == "No bookmarks saved.\n");
}

// listBookmarks prints only non-hidden bookmarks with 1-based numbers.
void test_list_shows_visible_only()
{
    const std::string lines[] = {"abc", "def"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    addBookmark(ed, "top", 0, 0);
    addBookmark(ed, "mid", 1, 2);
    ed.cursorLine = 0;
    ed.cursorCol = 1;
    savePreviousPosition(ed);
    std::ostringstream out;
    listBookmarks(ed, out);
    std::string s = out.str();
    CHECK(s.find("top") != std::string::npos);
    CHECK(s.find("mid") != std::string::npos);
    CHECK(s.find("__previous") == std::string::npos);
    CHECK(s.find("1. top") != std::string::npos);
    CHECK(s.find("2. mid") != std::string::npos);
    CHECK(s.find("(Line 1, Column 1)") != std::string::npos);
    CHECK(s.find("(Line 2, Column 3)") != std::string::npos);
}

// listBookmarks on a doc with only a hidden bookmark prints the empty message.
void test_list_only_hidden_prints_empty()
{
    const std::string lines[] = {"a"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    ed.cursorLine = 0;
    ed.cursorCol = 0;
    savePreviousPosition(ed);
    std::ostringstream out;
    listBookmarks(ed, out);
    CHECK(out.str() == "No bookmarks saved.\n");
}

// clearBookmarks removes everything, including hidden.
void test_clear_removes_all()
{
    const std::string lines[] = {"a", "b"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    addBookmark(ed, "x", 0, 0);
    addBookmark(ed, "y", 1, 0);
    ed.cursorLine = 0;
    ed.cursorCol = 0;
    savePreviousPosition(ed);
    CHECK(getBookmarkCount(ed) == 3);
    clearBookmarks(ed);
    CHECK(getBookmarkCount(ed) == 0);
    CHECK(findBookmark(ed, "x") == -1);
    CHECK(findBookmark(ed, "__previous") == -1);
}

// ---------- adjustBookmarksOnLineInsert ----------

// Inserting above shifts bookmarks at/after insertedLine down by 1.
void test_adjust_insert_shifts_lines()
{
    const std::string lines[] = {"a", "b", "c"};
    Editor ed;
    makeEditorWith(lines, 3, ed);
    addBookmark(ed, "before", 0, 0);
    addBookmark(ed, "at", 1, 0);
    addBookmark(ed, "after", 2, 0);
    adjustBookmarksOnLineInsert(ed, 1);
    Bookmark b;
    CHECK(ed.bookmarks.get(findBookmark(ed, "before"), b) && b.line == 0);
    CHECK(ed.bookmarks.get(findBookmark(ed, "at"), b) && b.line == 2);
    CHECK(ed.bookmarks.get(findBookmark(ed, "after"), b) && b.line == 3);
}

// Inserting at line 0 shifts every bookmark down by 1.
void test_adjust_insert_at_zero()
{
    const std::string lines[] = {"a", "b"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    addBookmark(ed, "p", 0, 0);
    addBookmark(ed, "q", 1, 0);
    adjustBookmarksOnLineInsert(ed, 0);
    Bookmark b;
    CHECK(ed.bookmarks.get(findBookmark(ed, "p"), b) && b.line == 1);
    CHECK(ed.bookmarks.get(findBookmark(ed, "q"), b) && b.line == 2);
}

// Inserting past every bookmark leaves them untouched.
void test_adjust_insert_past_all()
{
    const std::string lines[] = {"a", "b", "c"};
    Editor ed;
    makeEditorWith(lines, 3, ed);
    addBookmark(ed, "p", 0, 0);
    addBookmark(ed, "q", 1, 0);
    adjustBookmarksOnLineInsert(ed, 99);
    Bookmark b;
    CHECK(ed.bookmarks.get(findBookmark(ed, "p"), b) && b.line == 0);
    CHECK(ed.bookmarks.get(findBookmark(ed, "q"), b) && b.line == 1);
}

// Negative insertedLine is ignored.
void test_adjust_insert_negative_ignored()
{
    const std::string lines[] = {"a"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    addBookmark(ed, "p", 0, 0);
    adjustBookmarksOnLineInsert(ed, -1);
    Bookmark b;
    CHECK(ed.bookmarks.get(findBookmark(ed, "p"), b) && b.line == 0);
}

// ---------- adjustBookmarksOnLineDelete ----------

// Deleting a line removes the bookmark on it and shifts those below up.
void test_adjust_delete_removes_and_shifts()
{
    const std::string lines[] = {"a", "b", "c"};
    Editor ed;
    makeEditorWith(lines, 3, ed);
    addBookmark(ed, "before", 0, 0);
    addBookmark(ed, "hit", 1, 0);
    addBookmark(ed, "after", 2, 0);
    adjustBookmarksOnLineDelete(ed, 1);
    CHECK(findBookmark(ed, "hit") == -1);
    CHECK(getBookmarkCount(ed) == 2);
    Bookmark b;
    CHECK(ed.bookmarks.get(findBookmark(ed, "before"), b) && b.line == 0);
    CHECK(ed.bookmarks.get(findBookmark(ed, "after"), b) && b.line == 1);
}

// Deleting the last line removes a bookmark on it.
void test_adjust_delete_last_line()
{
    const std::string lines[] = {"a", "b"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    addBookmark(ed, "first", 0, 0);
    addBookmark(ed, "last", 1, 0);
    adjustBookmarksOnLineDelete(ed, 1);
    CHECK(findBookmark(ed, "last") == -1);
    CHECK(getBookmarkCount(ed) == 1);
    Bookmark b;
    CHECK(ed.bookmarks.get(findBookmark(ed, "first"), b) && b.line == 0);
}

// Deleting several bookmarked lines in one pass removes all without skipping.
void test_adjust_delete_several_in_one_pass()
{
    const std::string lines[] = {"a", "b", "c", "d", "e"};
    Editor ed;
    makeEditorWith(lines, 5, ed);
    addBookmark(ed, "b0", 0, 0);
    addBookmark(ed, "b1", 1, 0);
    addBookmark(ed, "b2", 2, 0);
    addBookmark(ed, "b3", 3, 0);
    addBookmark(ed, "b4", 4, 0);
    adjustBookmarksOnLineDelete(ed, 2);
    CHECK(findBookmark(ed, "b2") == -1);
    CHECK(getBookmarkCount(ed) == 4);
    Bookmark b;
    CHECK(ed.bookmarks.get(findBookmark(ed, "b0"), b) && b.line == 0);
    CHECK(ed.bookmarks.get(findBookmark(ed, "b1"), b) && b.line == 1);
    CHECK(ed.bookmarks.get(findBookmark(ed, "b3"), b) && b.line == 2);
    CHECK(ed.bookmarks.get(findBookmark(ed, "b4"), b) && b.line == 3);
}

// Negative deletedLine is ignored.
void test_adjust_delete_negative_ignored()
{
    const std::string lines[] = {"a"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    addBookmark(ed, "p", 0, 0);
    adjustBookmarksOnLineDelete(ed, -1);
    CHECK(getBookmarkCount(ed) == 1);
    Bookmark b;
    CHECK(ed.bookmarks.get(findBookmark(ed, "p"), b) && b.line == 0);
}

// Deleting a line below every bookmark shifts nothing.
void test_adjust_delete_below_all()
{
    const std::string lines[] = {"a", "b", "c"};
    Editor ed;
    makeEditorWith(lines, 3, ed);
    addBookmark(ed, "p", 0, 0);
    addBookmark(ed, "q", 1, 0);
    adjustBookmarksOnLineDelete(ed, 2);
    Bookmark b;
    CHECK(ed.bookmarks.get(findBookmark(ed, "p"), b) && b.line == 0);
    CHECK(ed.bookmarks.get(findBookmark(ed, "q"), b) && b.line == 1);
    CHECK(getBookmarkCount(ed) == 2);
}
// Split then join with the same arguments returns every bookmark to its original position.
void test_split_then_join_is_identity()
{
    const std::string lines[] = {"hello", "world", "foo"};
    Editor ed;
    makeEditorWith(lines, 3, ed);
    addBookmark(ed, "b1", 0, 0);
    addBookmark(ed, "b2", 0, 3); // exactly at split? no, split at 2
    addBookmark(ed, "b3", 1, 1);
    addBookmark(ed, "b4", 2, 2);

    adjustBookmarksOnSplit(ed, 0, 2);
    adjustBookmarksOnJoin(ed, 0, 2);

    Bookmark b;
    CHECK(ed.bookmarks.get(findBookmark(ed, "b1"), b) && b.line == 0 && b.col == 0);
    CHECK(ed.bookmarks.get(findBookmark(ed, "b2"), b) && b.line == 0 && b.col == 3);
    CHECK(ed.bookmarks.get(findBookmark(ed, "b3"), b) && b.line == 1 && b.col == 1);
    CHECK(ed.bookmarks.get(findBookmark(ed, "b4"), b) && b.line == 2 && b.col == 2);
}

// Bookmark exactly at the split column moves to the new line at col 0.
void test_split_bookmark_exactly_at_split_col()
{
    const std::string lines[] = {"abcdef"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    addBookmark(ed, "mid", 0, 3);

    adjustBookmarksOnSplit(ed, 0, 3);
    Bookmark b;
    CHECK(ed.bookmarks.get(findBookmark(ed, "mid"), b) && b.line == 1 && b.col == 0);
}

// Bookmark before the split col stays on the same line.
void test_split_bookmark_before_col()
{
    const std::string lines[] = {"abcdef"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    addBookmark(ed, "before", 0, 1);

    adjustBookmarksOnSplit(ed, 0, 3);
    Bookmark b;
    CHECK(ed.bookmarks.get(findBookmark(ed, "before"), b) && b.line == 0 && b.col == 1);
}

// Bookmark at line end (col == split col) moves to the new line at col 0.
void test_split_bookmark_at_line_end()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    addBookmark(ed, "end", 0, 3);

    adjustBookmarksOnSplit(ed, 0, 3);
    Bookmark b;
    CHECK(ed.bookmarks.get(findBookmark(ed, "end"), b) && b.line == 1 && b.col == 0);
}

// Join handles a bookmark on the merged-in line, adding the col offset.
void test_join_bookmark_on_second_half()
{
    const std::string lines[] = {"abc", "def"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    addBookmark(ed, "second", 1, 2);

    adjustBookmarksOnJoin(ed, 0, 3);
    Bookmark b;
    CHECK(ed.bookmarks.get(findBookmark(ed, "second"), b) && b.line == 0 && b.col == 5);
}

// Join moves bookmarks below line+1 up by one.
void test_join_bookmark_below()
{
    const std::string lines[] = {"a", "b", "c"};
    Editor ed;
    makeEditorWith(lines, 3, ed);
    addBookmark(ed, "below", 2, 0);

    adjustBookmarksOnJoin(ed, 0, 1);
    Bookmark b;
    CHECK(ed.bookmarks.get(findBookmark(ed, "below"), b) && b.line == 1);
}

// The hidden "__previous" slot participates in split like any other bookmark.
void test_split_handles_hidden_previous()
{
    const std::string lines[] = {"abcdef"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    ed.cursorLine = 0;
    ed.cursorCol = 4;
    savePreviousPosition(ed);

    adjustBookmarksOnSplit(ed, 0, 2);
    Bookmark b;
    int idx = findBookmark(ed, "__previous");
    CHECK(idx >= 0);
    CHECK(ed.bookmarks.get(idx, b) && b.line == 1 && b.col == 2);
}

// collectBookmarksInLines collects only the ones in range, including hidden.
void test_collect_bookmarks_in_lines()
{
    const std::string lines[] = {"a", "b", "c", "d"};
    Editor ed;
    makeEditorWith(lines, 4, ed);
    addBookmark(ed, "x", 0, 0);
    addBookmark(ed, "y", 1, 0);
    addBookmark(ed, "z", 2, 0);
    ed.cursorLine = 3;
    ed.cursorCol = 0;
    savePreviousPosition(ed);

    DynamicArray<Bookmark> out;
    collectBookmarksInLines(ed, 1, 2, out);
    CHECK(out.size() == 2);
    Bookmark b;
    out.get(0, b);
    CHECK(b.name == "y");
    out.get(1, b);
    CHECK(b.name == "z");
}

// collectBookmarksInLines accepts the hidden slot if its line is in range.
void test_collect_includes_hidden_when_in_range()
{
    const std::string lines[] = {"a", "b"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    ed.cursorLine = 1;
    ed.cursorCol = 0;
    savePreviousPosition(ed);

    DynamicArray<Bookmark> out;
    collectBookmarksInLines(ed, 1, 1, out);
    CHECK(out.size() == 1);
    Bookmark b;
    out.get(0, b);
    CHECK(b.name == "__previous");
}

// restoreBookmarks adds bookmarks that don't exist yet.
void test_restore_adds_new_bookmarks()
{
    const std::string lines[] = {"hello", "world"};
    Editor ed;
    makeEditorWith(lines, 2, ed);

    DynamicArray<Bookmark> saved;
    Bookmark b1;
    b1.name = "a";
    b1.line = 0;
    b1.col = 1;
    saved.pushBack(b1);
    Bookmark b2;
    b2.name = "b";
    b2.line = 1;
    b2.col = 2;
    saved.pushBack(b2);

    restoreBookmarks(ed, saved);
    CHECK(findBookmark(ed, "a") != -1);
    CHECK(findBookmark(ed, "b") != -1);
}

// restoreBookmarks skips duplicates (existing names).
void test_restore_skips_duplicates()
{
    const std::string lines[] = {"hello", "world"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    addBookmark(ed, "existing", 0, 0);

    DynamicArray<Bookmark> saved;
    Bookmark dup;
    dup.name = "existing";
    dup.line = 1;
    dup.col = 2;
    saved.pushBack(dup);
    Bookmark fresh;
    fresh.name = "fresh";
    fresh.line = 1;
    fresh.col = 1;
    saved.pushBack(fresh);

    restoreBookmarks(ed, saved);
    CHECK(findBookmark(ed, "existing") == 0); // unchanged
    Bookmark b;
    CHECK(ed.bookmarks.get(0, b) && b.line == 0 && b.col == 0); // original position
    CHECK(findBookmark(ed, "fresh") != -1);
}

// restoreBookmarks skips bookmarks whose line no longer exists.
void test_restore_skips_invalid_line()
{
    const std::string lines[] = {"only"};
    Editor ed;
    makeEditorWith(lines, 1, ed);

    DynamicArray<Bookmark> saved;
    Bookmark bad;
    bad.name = "bad";
    bad.line = 5;
    bad.col = 0;
    saved.pushBack(bad);
    Bookmark ok;
    ok.name = "ok";
    ok.line = 0;
    ok.col = 1;
    saved.pushBack(ok);

    restoreBookmarks(ed, saved);
    CHECK(findBookmark(ed, "bad") == -1);
    CHECK(findBookmark(ed, "ok") != -1);
}

// restoreBookmarks clamps col to line length.
void test_restore_clamps_col()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);

    DynamicArray<Bookmark> saved;
    Bookmark b;
    b.name = "long";
    b.line = 0;
    b.col = 99;
    saved.pushBack(b);

    restoreBookmarks(ed, saved);
    int idx = findBookmark(ed, "long");
    CHECK(idx != -1);
    Bookmark got;
    CHECK(ed.bookmarks.get(idx, got) && got.col == 3);
}

// restoreBookmarks can restore the hidden "__previous" slot (bypasses addBookmark).
void test_restore_accepts_hidden_name()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);

    DynamicArray<Bookmark> saved;
    Bookmark b;
    b.name = "__previous";
    b.line = 0;
    b.col = 1;
    saved.pushBack(b);

    restoreBookmarks(ed, saved);
    CHECK(findBookmark(ed, "__previous") != -1);
}

int main()
{
    test_add_and_find_bookmark();
    test_add_empty_name_rejected();
    test_add_whitespace_name_rejected();
    test_add_name_without_whitespace_accepted();
    test_add_reserved_name_rejected();
    test_add_duplicate_name_rejected();
    test_add_bad_line_rejected();
    test_add_bad_col_rejected_and_end_of_line_accepted();
    test_add_at_cursor_uses_editor_cursor();
    test_add_at_cursor_rejects_bad_cursor();

    test_find_unknown_returns_minus_one();
    test_remove_existing();
    test_remove_unknown();

    test_goto_sets_cursor();
    test_goto_clamps_col_when_line_shrank();
    test_goto_unknown_leaves_cursor();
    test_goto_returns_false_when_line_gone();
    test_goto_by_index_sets_cursor();
    test_goto_by_index_bad_index();
    test_goto_by_index_clamps_col();

    test_save_previous_stores_position();
    test_save_previous_updates_existing();
    test_save_previous_rejects_bad_cursor();
    test_return_previous_jumps();
    test_return_previous_missing();

    test_count_includes_hidden();
    test_list_empty();
    test_list_shows_visible_only();
    test_list_only_hidden_prints_empty();
    test_clear_removes_all();

    test_adjust_insert_shifts_lines();
    test_adjust_insert_at_zero();
    test_adjust_insert_past_all();
    test_adjust_insert_negative_ignored();

    test_adjust_delete_removes_and_shifts();
    test_adjust_delete_last_line();
    test_adjust_delete_several_in_one_pass();
    test_adjust_delete_negative_ignored();
    test_adjust_delete_below_all();

    test_split_then_join_is_identity();
    test_split_bookmark_exactly_at_split_col();
    test_split_bookmark_before_col();
    test_split_bookmark_at_line_end();
    test_join_bookmark_on_second_half();
    test_join_bookmark_below();
    test_split_handles_hidden_previous();
    test_collect_bookmarks_in_lines();
    test_collect_includes_hidden_when_in_range();
    test_restore_adds_new_bookmarks();
    test_restore_skips_duplicates();
    test_restore_skips_invalid_line();
    test_restore_clamps_col();
    test_restore_accepts_hidden_name();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}