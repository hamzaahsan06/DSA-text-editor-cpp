// g++ -std=c++11 -Wall -Wextra -Iinclude -Iinclude/editor tests/editor/test_Display.cpp src/editor/Display.cpp -o build/test_Display ; if ($?) { ./build/test_Display }

#include <iostream>
#include <sstream>
#include <string>
#include "editor/Display.h"
#include "editor/Editor.h"
#include "editor/Types.h"
#include "../ds/check.h"

// Fills `ed` with the given lines. Editor is non-copyable, so fill in place.
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

// Builds an editor with `n` numbered lines: "1", "2", ..., n.
static void makeNumberedEditor(int n, Editor &ed)
{
    ed.reset();
    if (n <= 0)
    {
        ed.document.clear();
        return;
    }
    Line *first = ed.getLine(0);
    if (first != nullptr)
        first->text = "1";
    for (int i = 2; i <= n; ++i)
    {
        Line l;
        // Simple int-to-string (no std::to_string to stay C++11-friendly).
        std::string s;
        int v = i;
        while (v > 0)
        {
            s = (char)('0' + v % 10) + s;
            v /= 10;
        }
        l.text = s;
        ed.document.insertAtTail(l);
    }
}

// ---------- getStats ----------

// Empty document has zero counts.
void test_stats_empty()
{
    Editor ed;
    ed.document.clear();
    DocStats s = getStats(ed);
    CHECK(s.lines == 0);
    CHECK(s.words == 0);
    CHECK(s.chars == 0);
    CHECK(s.charsNoSpaces == 0);
}

// Counts lines, words, chars correctly.
void test_stats_basic()
{
    const std::string lines[] = {"hello world", "one two three"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    DocStats s = getStats(ed);
    CHECK(s.lines == 2);
    CHECK(s.words == 5);
    CHECK(s.chars == 24);         // 11 + 13
    CHECK(s.charsNoSpaces == 21); // 10 + 11
}

// Empty lines count as lines but have no words.
void test_stats_empty_lines()
{
    const std::string lines[] = {"a", "", "b"};
    Editor ed;
    makeEditorWith(lines, 3, ed);
    DocStats s = getStats(ed);
    CHECK(s.lines == 3);
    CHECK(s.words == 2);
    CHECK(s.chars == 2);
}

// ---------- displayDocument ----------

// Empty document prints the placeholder.
void test_document_empty()
{
    Editor ed;
    ed.document.clear();
    std::ostringstream out;
    displayDocument(ed, out);
    CHECK(out.str() == "(document is empty)\n");
}

// Basic document prints each line with a 1-based label.
void test_document_basic()
{
    const std::string lines[] = {"hello", "world"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    std::ostringstream out;
    displayDocument(ed, out);
    CHECK(out.str() ==
          "1 | hello\n"
          "2 | world\n");
}

// ---------- displayLine ----------

// Prints a single line with the correct label.
void test_line_basic()
{
    const std::string lines[] = {"a", "b", "c"};
    Editor ed;
    makeEditorWith(lines, 3, ed);
    std::ostringstream out;
    CHECK(displayLine(ed, 1, out) == true);
    CHECK(out.str() == "2 | b\n");
}

// Bad index prints an error and returns false.
void test_line_bad_index()
{
    const std::string lines[] = {"a"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    std::ostringstream out;
    CHECK(displayLine(ed, 5, out) == false);
    CHECK(out.str() == "Error: line 6 does not exist (document has 1 lines).\n");
}

// Empty document prints an error and returns false.
void test_line_empty_document()
{
    Editor ed;
    ed.document.clear();
    std::ostringstream out;
    CHECK(displayLine(ed, 0, out) == false);
    CHECK(out.str() == "Error: the document is empty.\n");
}

// ---------- displayStats ----------

// Prints the exact stat block including filename and modified flag.
void test_stats_display()
{
    const std::string lines[] = {"hello world"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    ed.setFilename("test.txt");
    ed.setModified(true);
    std::ostringstream out;
    displayStats(ed, out);
    CHECK(out.str() ==
          "--- Document info ---\n"
          "File       : test.txt\n"
          "Lines      : 1\n"
          "Words      : 2\n"
          "Characters : 11 (10 without spaces)\n"
          "Modified   : yes\n");
}

// Unsaved file shows "(unsaved)".
void test_stats_display_unsaved()
{
    const std::string lines[] = {"hi"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    std::ostringstream out;
    displayStats(ed, out);
    CHECK(out.str().find("(unsaved)") != std::string::npos);
}

// ---------- displayStatusBar (new) ----------

// Status bar without a filename shows "[untitled]".
void test_status_bar_untitled()
{
    const std::string lines[] = {"hello"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    std::ostringstream out;
    displayStatusBar(ed, out);
    CHECK(out.str().find("[untitled]") != std::string::npos);
}

// Status bar with filename shows it.
void test_status_bar_with_filename()
{
    const std::string lines[] = {"hello"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    ed.setFilename("notes.txt");
    std::ostringstream out;
    displayStatusBar(ed, out);
    CHECK(out.str().find("notes.txt") != std::string::npos);
    CHECK(out.str().find("[modified]") == std::string::npos);
}

// Modified flag appears only when true.
void test_status_bar_modified()
{
    const std::string lines[] = {"x"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    ed.setModified(true);
    std::ostringstream out;
    displayStatusBar(ed, out);
    CHECK(out.str().find(" [modified]") != std::string::npos);
}

// Status bar shows Ln/Col and undo/redo counts exactly.
void test_status_bar_line_col_and_undo_counts()
{
    const std::string lines[] = {"aaa", "bbb", "ccc"};
    Editor ed;
    makeEditorWith(lines, 3, ed);
    ed.cursorLine = 1;
    ed.cursorCol = 2;
    // Fake undo/redo entries: push dummy Commands.
    Command dummy;
    ed.undoStack.push(dummy);
    ed.undoStack.push(dummy);
    ed.redoStack.push(dummy);
    std::ostringstream out;
    displayStatusBar(ed, out);
    CHECK(out.str().find("Ln 2/3, Col 3") != std::string::npos);
    CHECK(out.str().find("undo 2, redo 1") != std::string::npos);
}

// Zero-line document shows "Ln -, Col -".
void test_status_bar_empty_document()
{
    Editor ed;
    ed.document.clear();
    std::ostringstream out;
    displayStatusBar(ed, out);
    CHECK(out.str().find("Ln -, Col -") != std::string::npos);
}

// ---------- displayRange (new) ----------

// Displays a contiguous range with correct widths.
void test_range_basic()
{
    makeNumberedEditor(10, *(new Editor));
    // Recreate editor to avoid leaks.
    Editor ed;
    makeNumberedEditor(10, ed);
    std::ostringstream out;
    CHECK(displayRange(ed, 1, 3, false, out) == true);
    CHECK(out.str() ==
          " 2 | 2\n"
          " 3 | 3\n"
          " 4 | 4\n");
}

// markCursor adds a '>' to the cursor line.
void test_range_mark_cursor()
{
    Editor ed;
    makeNumberedEditor(3, ed);
    ed.cursorLine = 1;
    std::ostringstream out;
    CHECK(displayRange(ed, 0, 2, true, out) == true);
    CHECK(out.str() ==
          " 1 | 1\n"
          ">2 | 2\n"
          " 3 | 3\n");
}

// Invalid range prints an error.
void test_range_invalid()
{
    Editor ed;
    makeNumberedEditor(2, ed);
    std::ostringstream out;
    CHECK(displayRange(ed, 1, 5, false, out) == false);
    CHECK(out.str().find("invalid line range") != std::string::npos);
}

// Reversed range (end < start) is rejected.
void test_range_reversed()
{
    Editor ed;
    makeNumberedEditor(5, ed);
    std::ostringstream out;
    CHECK(displayRange(ed, 3, 1, false, out) == false);
    CHECK(out.str().find("invalid line range") != std::string::npos);
}

// Negative start is rejected.
void test_range_negative_start()
{
    Editor ed;
    makeNumberedEditor(5, ed);
    std::ostringstream out;
    CHECK(displayRange(ed, -1, 2, false, out) == false);
    CHECK(out.str().find("invalid line range") != std::string::npos);
}

// Empty document prints an error.
void test_range_empty_document()
{
    Editor ed;
    ed.document.clear();
    std::ostringstream out;
    CHECK(displayRange(ed, 0, 0, false, out) == false);
    CHECK(out.str() == "Error: the document is empty.\n");
}

// Number alignment past line 9 (width 2).
void test_range_alignment_past_9()
{
    Editor ed;
    makeNumberedEditor(10, ed);
    std::ostringstream out;
    CHECK(displayRange(ed, 8, 9, false, out) == true);
    CHECK(out.str() ==
          " 9 | 9\n"
          "10 | 10\n");
}

// Number alignment past line 99 (width 3).
void test_range_alignment_past_99()
{
    Editor ed;
    makeNumberedEditor(100, ed);
    std::ostringstream out;
    CHECK(displayRange(ed, 98, 99, false, out) == true);
    CHECK(out.str() ==
          " 99 | 99\n"
          "100 | 100\n");
}

// ---------- displayWindow (new) ----------

// Empty document prints the placeholder.
void test_window_empty_document()
{
    Editor ed;
    ed.document.clear();
    std::ostringstream out;
    displayWindow(ed, 5, out);
    CHECK(out.str() == "(document is empty)\n");
}

// Exactly maxLines lines: all printed, no ellipsis.
void test_window_exactly_maxLines()
{
    Editor ed;
    makeNumberedEditor(5, ed);
    std::ostringstream out;
    displayWindow(ed, 5, out);
    CHECK(out.str().find("more line(s)") == std::string::npos);
    CHECK(out.str().find(" 5 | 5") != std::string::npos);
}

// maxLines + 1 lines: one line is hidden below.
void test_window_maxLines_plus_one()
{
    Editor ed;
    makeNumberedEditor(6, ed);
    ed.cursorLine = 0;
    std::ostringstream out;
    displayWindow(ed, 5, out);
    CHECK(out.str().find("more line(s) above") == std::string::npos);
    CHECK(out.str().find("... 1 more line(s) below") != std::string::npos);
}

// 100 lines, cursor at start: no "above", shows window, shows "below".
void test_window_100_cursor_start()
{
    Editor ed;
    makeNumberedEditor(100, ed);
    ed.cursorLine = 0;
    std::ostringstream out;
    displayWindow(ed, 5, out);
    CHECK(out.str().find("more line(s) above") == std::string::npos);
    CHECK(out.str().find("more line(s) below") != std::string::npos);
}

// 100 lines, cursor in the middle: shows both ellipsis markers.
void test_window_100_cursor_middle()
{
    Editor ed;
    makeNumberedEditor(100, ed);
    ed.cursorLine = 50;
    std::ostringstream out;
    displayWindow(ed, 5, out);
    CHECK(out.str().find("more line(s) above") != std::string::npos);
    CHECK(out.str().find("more line(s) below") != std::string::npos);
}

// 100 lines, cursor at end: shows "above", no "below".
void test_window_100_cursor_end()
{
    Editor ed;
    makeNumberedEditor(100, ed);
    ed.cursorLine = 99;
    std::ostringstream out;
    displayWindow(ed, 5, out);
    CHECK(out.str().find("more line(s) above") != std::string::npos);
    CHECK(out.str().find("more line(s) below") == std::string::npos);
}

// Cursor beyond document is treated as last line.
void test_window_cursor_out_of_range()
{
    Editor ed;
    makeNumberedEditor(5, ed);
    ed.cursorLine = 100;
    std::ostringstream out;
    displayWindow(ed, 3, out);
    CHECK(out.str().find(" 5 | 5") != std::string::npos);
}

// maxLines <= 0 prints "... N more line(s) below".
void test_window_zero_maxLines()
{
    Editor ed;
    makeNumberedEditor(3, ed);
    std::ostringstream out;
    displayWindow(ed, 0, out);
    CHECK(out.str() == "... 3 more line(s) below\n");
}

int main()
{
    // Existing tests
    test_stats_empty();
    test_stats_basic();
    test_stats_empty_lines();
    test_document_empty();
    test_document_basic();
    test_line_basic();
    test_line_bad_index();
    test_line_empty_document();
    test_stats_display();
    test_stats_display_unsaved();

    // New tests
    test_status_bar_untitled();
    test_status_bar_with_filename();
    test_status_bar_modified();
    test_status_bar_line_col_and_undo_counts();
    test_status_bar_empty_document();

    test_range_basic();
    test_range_mark_cursor();
    test_range_invalid();
    test_range_reversed();
    test_range_negative_start();
    test_range_empty_document();
    test_range_alignment_past_9();
    test_range_alignment_past_99();

    test_window_empty_document();
    test_window_exactly_maxLines();
    test_window_maxLines_plus_one();
    test_window_100_cursor_start();
    test_window_100_cursor_middle();
    test_window_100_cursor_end();
    test_window_cursor_out_of_range();
    test_window_zero_maxLines();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}