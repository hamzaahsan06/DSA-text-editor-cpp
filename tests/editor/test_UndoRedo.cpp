// g++ -std=c++11 -Wall -Wextra -Iinclude -Iinclude/editor tests/editor/test_UndoRedo.cpp src/editor/UndoRedo.cpp src/editor/Bookmarks.cpp src/editor/Formatting.cpp -o build/test_UndoRedo ; if ($?) { ./build/test_UndoRedo }

#include <iostream>
#include <string>
#include "editor/UndoRedo.h"
#include "editor/Bookmarks.h"
#include "editor/Editor.h"
#include "editor/Types.h"
#include "editor/Formatting.h"
#include "../ds/check.h"

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

// Line edit undo/redo restores text and formats.
void test_line_edit_restores_formats_and_cursor()
{
    const std::string lines[] = {"hello"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    UndoRedo ur;

    Line before;
    before.text = "hello";
    FormatSpan fs;
    fs.startCol = 0;
    fs.endCol = 5;
    fs.type = BOLD;
    before.formats.pushBack(fs);

    Position cb;
    cb.line = 0;
    cb.col = 2;

    // Edit in place
    Line *l = ed.getLine(0);
    l->text = "world";
    clearFormats(*l);
    FormatSpan fs2;
    fs2.startCol = 1;
    fs2.endCol = 4;
    fs2.type = ITALIC;
    l->formats.pushBack(fs2);
    ed.cursorLine = 0;
    ed.cursorCol = 4;

    CHECK(ur.recordLineEdit(ed, INSERT_TEXT, 0, before, cb) == true);

    // Undo
    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLine(0)->text == "hello");
    CHECK(ed.getLine(0)->formats.size() == 1);
    FormatSpan out;
    ed.getLine(0)->formats.get(0, out);
    CHECK(out.type == BOLD && out.startCol == 0 && out.endCol == 5);
    CHECK(ed.cursorLine == 0 && ed.cursorCol == 2);

    // Redo
    CHECK(ur.redo(ed) == true);
    CHECK(ed.getLine(0)->text == "world");
    CHECK(ed.getLine(0)->formats.size() == 1);
    ed.getLine(0)->formats.get(0, out);
    CHECK(out.type == ITALIC && out.startCol == 1 && out.endCol == 4);
    CHECK(ed.cursorLine == 0 && ed.cursorCol == 4);
}

// Split undo/redo with formats, cursor, and bookmark.
void test_split_undo_redo_with_bookmark()
{
    const std::string lines[] = {"hello world"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    UndoRedo ur;

    addBookmark(ed, "mark", 0, 8); // points to 'r'
    Position cb;
    cb.line = 0;
    cb.col = 5;

    Line before;
    before.text = "hello world";
    FormatSpan fs;
    fs.startCol = 0;
    fs.endCol = 11;
    fs.type = BOLD;
    before.formats.pushBack(fs);

    // Perform Split
    ed.document.insertAtTail(Line());
    ed.getLine(0)->text = "hello ";
    ed.getLine(1)->text = "world";

    ed.cursorLine = 1;
    ed.cursorCol = 0;

    CHECK(ur.recordSplit(ed, 0, 6, before, cb) == true);

    // Undo
    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLineCount() == 1);
    CHECK(ed.getLine(0)->text == "hello world");
    CHECK(findBookmark(ed, "mark") == 0);
    Bookmark b;
    ed.bookmarks.get(0, b);
    CHECK(b.line == 0 && b.col == 8);
    CHECK(ed.cursorLine == 0 && ed.cursorCol == 5);

    // Redo
    CHECK(ur.redo(ed) == true);
    CHECK(ed.getLineCount() == 2);
    CHECK(ed.getLine(0)->text == "hello ");
    CHECK(ed.getLine(1)->text == "world");
    CHECK(findBookmark(ed, "mark") == 0);
    ed.bookmarks.get(0, b);
    CHECK(b.line == 1 && b.col == 2); // moved to line 1, col 8-6
    CHECK(ed.cursorLine == 1 && ed.cursorCol == 0);
}

// Delete lines undo/redo (single and range).
void test_delete_lines_undo_redo()
{
    const std::string lines[] = {"one", "two", "three", "four"};
    Editor ed;
    makeEditorWith(lines, 4, ed);
    UndoRedo ur;

    addBookmark(ed, "b_two", 1, 0);
    addBookmark(ed, "b_three", 2, 0);
    addBookmark(ed, "b_four", 3, 0);

    DynamicArray<Line> deleted;
    deleted.pushBack(*ed.getLine(1));
    deleted.pushBack(*ed.getLine(2));

    DynamicArray<Bookmark> removed;
    removed.pushBack(ed.bookmarks[0]); // b_two
    removed.pushBack(ed.bookmarks[1]); // b_three

    Position cb;
    cb.line = 0;
    cb.col = 0;

    // Perform delete
    Line dummy;
    ed.document.removeAtPosition(1, dummy);
    ed.document.removeAtPosition(1, dummy);
    adjustBookmarksOnLineDelete(ed, 1);
    adjustBookmarksOnLineDelete(ed, 1);
    ed.cursorLine = 1;
    ed.cursorCol = 0;

    CHECK(ur.recordDeleteLines(ed, 1, deleted, removed, cb) == true);

    // Undo
    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLineCount() == 4);
    CHECK(ed.getLine(1)->text == "two");
    CHECK(ed.getLine(2)->text == "three");
    CHECK(findBookmark(ed, "b_two") != -1);
    CHECK(findBookmark(ed, "b_three") != -1);
    Bookmark b;
    ed.bookmarks.get(findBookmark(ed, "b_four"), b);
    CHECK(b.line == 3); // shifted back
    CHECK(ed.cursorLine == 0 && ed.cursorCol == 0);

    // Redo
    CHECK(ur.redo(ed) == true);
    CHECK(ed.getLineCount() == 2);
    CHECK(findBookmark(ed, "b_two") == -1);
    ed.bookmarks.get(findBookmark(ed, "b_four"), b);
    CHECK(b.line == 1); // shifted up
    CHECK(ed.cursorLine == 1 && ed.cursorCol == 0);
}

// Delete all lines.
void test_delete_all_lines()
{
    const std::string lines[] = {"a", "b"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    UndoRedo ur;

    DynamicArray<Line> deleted;
    deleted.pushBack(*ed.getLine(0));
    deleted.pushBack(*ed.getLine(1));

    DynamicArray<Bookmark> removed;

    Position cb;
    cb.line = 0;
    cb.col = 0;

    ed.document.clear();
    ed.cursorLine = 0;
    ed.cursorCol = 0;

    CHECK(ur.recordDeleteLines(ed, 0, deleted, removed, cb) == true);
    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLineCount() == 2);
    CHECK(ed.getLine(0)->text == "a");
    CHECK(ur.redo(ed) == true);
    CHECK(ed.getLineCount() == 0);
}

// recordBlockEdit with different sizes (e.g. Replace All).
void test_block_edit_different_sizes()
{
    const std::string lines[] = {"a", "b", "c", "d"};
    Editor ed;
    makeEditorWith(lines, 4, ed);
    UndoRedo ur;

    DynamicArray<Line> before;
    before.pushBack(*ed.getLine(1)); // "b"
    before.pushBack(*ed.getLine(2)); // "c"

    // Replace 2 lines with 3 lines
    Line dummy;
    ed.document.removeAtPosition(1, dummy);
    ed.document.removeAtPosition(1, dummy);
    Line n1;
    n1.text = "x";
    ed.document.insertAtPosition(1, n1);
    Line n2;
    n2.text = "y";
    ed.document.insertAtPosition(2, n2);
    Line n3;
    n3.text = "z";
    ed.document.insertAtPosition(3, n3);

    Position cb;
    cb.line = 0;
    cb.col = 0;
    CHECK(ur.recordBlockEdit(ed, REPLACE_ALL, 1, before, 3, cb) == true);

    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLineCount() == 4);
    CHECK(ed.getLine(1)->text == "b");
    CHECK(ed.getLine(2)->text == "c");

    CHECK(ur.redo(ed) == true);
    CHECK(ed.getLineCount() == 5);
    CHECK(ed.getLine(1)->text == "x");
    CHECK(ed.getLine(3)->text == "z");
}

// New record clears redo.
void test_new_record_clears_redo()
{
    Editor ed;
    UndoRedo ur;
    Line before;

    CHECK(ur.undo(ed) == false);
    CHECK(ur.redo(ed) == false);

    ur.recordLineEdit(ed, INSERT_TEXT, 0, before, Position());
    CHECK(ur.undo(ed) == true);
    CHECK(ur.canRedo(ed) == true);

    ur.recordLineEdit(ed, INSERT_TEXT, 0, before, Position());
    CHECK(ur.canRedo(ed) == false);
}

// Invalid arguments.
void test_invalid_arguments()
{
    Editor ed;
    UndoRedo ur;
    Line before;
    DynamicArray<Line> block;
    DynamicArray<Bookmark> bms;
    Position p;

    CHECK(ur.recordLineEdit(ed, INSERT_TEXT, 99, before, p) == false);
    CHECK(ur.recordSplit(ed, 0, 2, before, p) == false); // 0+1 doesn't exist
    CHECK(ur.recordBlockEdit(ed, REPLACE_ALL, 0, block, -1, p) == false);
    CHECK(ur.recordBlockEdit(ed, REPLACE_ALL, 0, block, 5, p) == false);
}

// 30 cycles of edit/undo/redo remain identical.
void test_30_cycles()
{
    const std::string lines[] = {"original"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    UndoRedo ur;

    for (int i = 0; i < 30; ++i)
    {
        Line before = *ed.getLine(0);
        ed.getLine(0)->text = "edited";
        ur.recordLineEdit(ed, INSERT_TEXT, 0, before, Position());

        CHECK(ur.undo(ed) == true);
        CHECK(ed.getLine(0)->text == "original");

        CHECK(ur.redo(ed) == true);
        CHECK(ed.getLine(0)->text == "edited");

        // Reset back to original manually for the next cycle
        ed.getLine(0)->text = "original";
        ur.clearHistory(ed);
    }
}

int main()
{
    test_line_edit_restores_formats_and_cursor();
    test_split_undo_redo_with_bookmark();
    test_delete_lines_undo_redo();
    test_delete_all_lines();
    test_block_edit_different_sizes();
    test_new_record_clears_redo();
    test_invalid_arguments();
    test_30_cycles();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}