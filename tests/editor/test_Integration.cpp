// g++ -std=c++11 -Wall -Wextra -Iinclude -Iinclude/editor tests/editor/test_Integration.cpp src/editor/TextOps.cpp src/editor/SearchReplace.cpp src/editor/Bookmarks.cpp src/editor/UndoRedo.cpp src/editor/FileManager.cpp src/editor/Display.cpp src/editor/Formatting.cpp src/editor/Validation.cpp -o build/test_Integration ; if ($?) { ./build/test_Integration }

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstdio>
#include "editor/TextOps.h"
#include "editor/SearchReplace.h"
#include "editor/Bookmarks.h"
#include "editor/UndoRedo.h"
#include "editor/FileManager.h"
#include "editor/Display.h"
#include "editor/Formatting.h"
#include "editor/Editor.h"
#include "editor/Types.h"
#include "../ds/check.h"

// Builds an editor with the given lines.
static void makeEditorWith(const std::string* lines, int n, Editor& ed)
{
    ed.reset();
    if (n <= 0) return;
    Line* first = ed.getLine(0);
    if (first != nullptr) first->text = lines[0];
    for (int i = 1; i < n; ++i) {
        Line l; l.text = lines[i];
        ed.document.insertAtTail(l);
    }
}

// Writes a raw file (used by test 10).
static void writeRawFile(const std::string& path, const std::string& content)
{
    std::ofstream out(path.c_str());
    out << content;
    out.close();
}

// Applies a format span AND records it for undo.
static bool applyFormatRecorded(Editor& ed, int line, int startCol, int endCol,
                                bool isBold, bool isItalic)
{
    Line* l = ed.getLine(line);
    if (l == nullptr) return false;
    Position cb; cb.line = ed.cursorLine; cb.col = ed.cursorCol;
    Line before = *l;
    if (!addFormat(*l, startCol, endCol, isBold, isItalic)) return false;
    ed.setModified(true);
    UndoRedo history;
    return history.recordLineEdit(ed, FORMAT_APPLY, line, before, cb);
}

// Serializes the whole document (text + spans) into one string for exact comparison.
static std::string dumpDocument(const Editor& ed)
{
    std::string out = "";
    for (int i = 0; i < ed.getLineCount(); ++i) {
        Line l;
        if (!ed.document.get(i, l)) continue;
        out += "Line " + std::to_string(i) + ": [" + l.text + "] ";
        for (int j = 0; j < l.formats.size(); ++j) {
            FormatSpan s;
            if (!l.formats.get(j, s)) continue;
            out += "(" + std::to_string(s.startCol) + "-" + std::to_string(s.endCol) + " ";
            if (s.type == BOLD) out += "B";
            else if (s.type == ITALIC) out += "I";
            else out += "BI";
            out += ") ";
        }
        out += "\n";
    }
    return out;
}

// Ensures the cursor is in a valid position and all bookmarks are valid.
static bool isStateValid(Editor& ed)
{
    if (ed.getLineCount() == 0) {
        if (ed.cursorLine != 0 || ed.cursorCol != 0) return false;
    } else {
        if (ed.cursorLine < 0 || ed.cursorLine >= ed.getLineCount()) return false;
        Line* l = ed.getLine(ed.cursorLine);
        if (!l || ed.cursorCol < 0 || ed.cursorCol > (int)l->text.length()) return false;
    }
    for (int i = 0; i < ed.bookmarks.size(); ++i) {
        Bookmark b;
        if (!ed.bookmarks.get(i, b)) continue;
        if (b.line < 0 || b.line >= ed.getLineCount()) return false;
        Line* l = ed.getLine(b.line);
        if (!l || b.col < 0 || b.col > (int)l->text.length()) return false;
    }
    return true;
}

// 1. Replace All "ab" -> "xyz", undo/redo.
void test_1_replace_all_undo_redo()
{
    const std::string lines[] = { "ab ab ab" };
    Editor ed; makeEditorWith(lines, 1, ed);
    std::string orig = dumpDocument(ed);

    CHECK(replaceAll(ed, "ab", "xyz") == 3);
    CHECK(ed.getLine(0)->text == "xyz xyz xyz");
    CHECK(isStateValid(ed));

    UndoRedo ur;
    CHECK(ur.undo(ed) == true);
    CHECK(dumpDocument(ed) == orig);
    CHECK(isStateValid(ed));

    CHECK(ur.redo(ed) == true);
    CHECK(ed.getLine(0)->text == "xyz xyz xyz");
    CHECK(isStateValid(ed));
}

// 2. Replace All across 3 lines with formats.
void test_2_replace_all_multiline_formats()
{
    const std::string lines[] = { "ab cat", "longer ab line", "ab" };
    Editor ed; makeEditorWith(lines, 3, ed);
    applyFormatRecorded(ed, 0, 3, 6, true, false); // Bold "cat"
    std::string orig = dumpDocument(ed);

    CHECK(replaceAll(ed, "ab", "xyz") == 3);
    CHECK(ed.getLine(0)->text == "xyz cat");
    CHECK(ed.getLine(1)->text == "longer xyz line");
    CHECK(isStateValid(ed));

    Line* l = ed.getLine(0);
    CHECK(l->formats.size() == 1);
    FormatSpan s; l->formats.get(0, s);
    CHECK(s.startCol == 4 && s.endCol == 7); // shifted right by 1

    UndoRedo ur;
    CHECK(ur.undo(ed) == true);
    CHECK(dumpDocument(ed) == orig);
    CHECK(isStateValid(ed));
}

// 3. Split a line with a bookmark in the moved half.
void test_3_split_with_bookmark()
{
    const std::string lines[] = { "hello world" };
    Editor ed; makeEditorWith(lines, 1, ed);
    addBookmark(ed, "mark", 0, 8);
    std::string orig = dumpDocument(ed);

    CHECK(insertNewLine(ed, 0, 5) == true);
    CHECK(ed.getLine(0)->text == "hello");
    CHECK(ed.getLine(1)->text == " world");
    
    UndoRedo ur;
    CHECK(ur.undo(ed) == true);
    CHECK(dumpDocument(ed) == orig);
    CHECK(isStateValid(ed));
    Bookmark b; ed.bookmarks.get(findBookmark(ed, "mark"), b);
    CHECK(b.line == 0 && b.col == 8);

    CHECK(ur.redo(ed) == true);
    CHECK(ed.getLineCount() == 2);
    CHECK(isStateValid(ed));
    ed.bookmarks.get(findBookmark(ed, "mark"), b);
    CHECK(b.line == 1 && b.col == 3); // moved to new line
}

// 4. Delete lines 2-3 with bookmarks on 2 and 4.
void test_4_delete_lines_bookmarks()
{
    const std::string lines[] = { "a", "b", "c", "d", "e" };
    Editor ed; makeEditorWith(lines, 5, ed);
    addBookmark(ed, "b2", 1, 0);
    addBookmark(ed, "b4", 3, 0);

    CHECK(deleteLines(ed, 1, 2) == true); // Deletes "b", "c"
    CHECK(findBookmark(ed, "b2") == -1);

    UndoRedo ur;
    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLineCount() == 5);
    int i2 = findBookmark(ed, "b2");
    int i4 = findBookmark(ed, "b4");
    CHECK(i2 >= 0 && i4 >= 0);
    Bookmark b; ed.bookmarks.get(i2, b); CHECK(b.line == 1);
    ed.bookmarks.get(i4, b); CHECK(b.line == 3);
    CHECK(isStateValid(ed));
}

// 5. Delete ALL lines, undo, redo, undo.
void test_5_delete_all_lines()
{
    const std::string lines[] = { "1", "2" };
    Editor ed; makeEditorWith(lines, 2, ed);
    std::string orig = dumpDocument(ed);

    CHECK(deleteLines(ed, 0, 1) == true);
    CHECK(ed.getLineCount() == 0);
    CHECK(isStateValid(ed));

    UndoRedo ur;
    CHECK(ur.undo(ed) == true);
    CHECK(dumpDocument(ed) == orig);

    CHECK(ur.redo(ed) == true);
    CHECK(ed.getLineCount() == 0);
    CHECK(isStateValid(ed));

    CHECK(ur.undo(ed) == true);
    CHECK(dumpDocument(ed) == orig);
}

// 6. Format a word, insert text before it, undo twice.
void test_6_format_insert_undo()
{
    const std::string lines[] = { "abcde" };
    Editor ed; makeEditorWith(lines, 1, ed);
    applyFormatRecorded(ed, 0, 2, 5, true, false);
    std::string orig = dumpDocument(ed);

    CHECK(insertText(ed, 0, 0, "XY") == true);
    CHECK(ed.getLine(0)->text == "XYabcde");

    UndoRedo ur;
    CHECK(ur.undo(ed) == true); // undo insert
    CHECK(dumpDocument(ed) == orig);

    CHECK(ur.undo(ed) == true); // undo format
    CHECK(ed.getLine(0)->formats.size() == 0);

    CHECK(ur.redo(ed) == true); // redo format
    CHECK(dumpDocument(ed) == orig);

    CHECK(ur.redo(ed) == true); // redo insert
    CHECK(ed.getLine(0)->text == "XYabcde");
    CHECK(isStateValid(ed));
}

// 7. Split a formatted line, undo.
void test_7_split_format_undo()
{
    const std::string lines[] = { "abcdef" };
    Editor ed; makeEditorWith(lines, 1, ed);
    applyFormatRecorded(ed, 0, 1, 4, false, true);
    std::string orig = dumpDocument(ed);

    CHECK(insertNewLine(ed, 0, 3) == true);
    CHECK(ed.getLine(0)->text == "abc");
    CHECK(ed.getLine(1)->text == "def");
    
    UndoRedo ur;
    CHECK(ur.undo(ed) == true);
    CHECK(dumpDocument(ed) == orig);
    CHECK(isStateValid(ed));
}

// 8. Long chain of 20 deterministic operations.
void test_8_long_chain()
{
    const std::string lines[] = { "hello world" };
    Editor ed; makeEditorWith(lines, 1, ed);

    DynamicArray<std::string> snapshots;
    snapshots.pushBack(dumpDocument(ed));

    // 10 deterministic edits (all valid, guaranteed to succeed)
    CHECK(insertText(ed, 0, 0, "A") == true);
    snapshots.pushBack(dumpDocument(ed));
    CHECK(insertText(ed, 0, 0, "B") == true);
    snapshots.pushBack(dumpDocument(ed));
    CHECK(deleteChar(ed, 0, 0) == true);
    snapshots.pushBack(dumpDocument(ed));
    CHECK(insertNewLine(ed, 0, 2) == true);
    snapshots.pushBack(dumpDocument(ed));
    CHECK(insertNewLine(ed, 1, 2) == true);
    snapshots.pushBack(dumpDocument(ed));
    CHECK(deleteLines(ed, 2, 2) == true);
    snapshots.pushBack(dumpDocument(ed));
    CHECK(insertText(ed, 0, 1, "Z") == true);
    snapshots.pushBack(dumpDocument(ed));
    CHECK(applyFormatRecorded(ed, 0, 0, 2, true, false) == true);
    snapshots.pushBack(dumpDocument(ed));
    CHECK(replaceAll(ed, "l", "L") == 1);
    snapshots.pushBack(dumpDocument(ed));
    CHECK(deleteLines(ed, 0, 0) == true);
    snapshots.pushBack(dumpDocument(ed));

    const int N = snapshots.size() - 1; // 10 commands recorded

    // Undo all 10 commands in reverse.
    UndoRedo ur;
    for (int i = N; i >= 1; --i) {
        CHECK(ur.undo(ed) == true);
        std::string expected;
        snapshots.get(i - 1, expected);
        CHECK(dumpDocument(ed) == expected);
        CHECK(isStateValid(ed));
    }

    // Redo all 10 commands forward.
    for (int i = 1; i <= N; ++i) {
        CHECK(ur.redo(ed) == true);
        std::string expected;
        snapshots.get(i, expected);
        CHECK(dumpDocument(ed) == expected);
        CHECK(isStateValid(ed));
    }
}

// 9. New edit clears redo.
void test_9_redo_cleared()
{
    Editor ed;
    UndoRedo ur;
    insertText(ed, 0, 0, "a");
    ur.undo(ed);
    CHECK(ur.canRedo(ed) == true);
    insertText(ed, 0, 0, "b");
    CHECK(ur.canRedo(ed) == false);
}

// 10. openFile and newDocument clear history.
void test_10_open_new_clears_history()
{
    const std::string path = "test_integ_tmp.txt";
    writeRawFile(path, "file content");

    Editor ed;
    insertText(ed, 0, 0, "hello");
    UndoRedo ur;
    CHECK(ur.canUndo(ed) == true);

    CHECK(openFile(ed, path) == true);
    CHECK(ur.canUndo(ed) == false);
    CHECK(getBookmarkCount(ed) == 0);

    insertText(ed, 0, 0, "x");
    CHECK(ur.canUndo(ed) == true);
    
    CHECK(newDocument(ed) == true);
    CHECK(ur.canUndo(ed) == false);

    std::remove(path.c_str());
}

// 11. Save and reopen keeps text identical.
void test_11_save_reopen()
{
    const std::string path = "test_integ_save_tmp.txt";
    const std::string lines[] = { "line one", "line two" };
    Editor ed; makeEditorWith(lines, 2, ed);
    
    CHECK(saveFileAs(ed, path) == true);

    Editor ed2;
    CHECK(openFile(ed2, path) == true);
    
    // Formats are NOT saved to disk. Text must match.
    CHECK(ed2.getLineCount() == 2);
    CHECK(ed2.getLine(0)->text == "line one");
    CHECK(ed2.getLine(1)->text == "line two");

    std::remove(path.c_str());
}

int main()
{
    test_1_replace_all_undo_redo();
    test_2_replace_all_multiline_formats();
    test_3_split_with_bookmark();
    test_4_delete_lines_bookmarks();
    test_5_delete_all_lines();
    test_6_format_insert_undo();
    test_7_split_format_undo();
    test_8_long_chain();
    test_9_redo_cleared();
    test_10_open_new_clears_history();
    test_11_save_reopen();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}