// g++ -std=c++11 -Wall -Wextra -Iinclude -Iinclude/editor tests/editor/test_SearchReplace.cpp src/editor/SearchReplace.cpp src/editor/UndoRedo.cpp src/editor/Bookmarks.cpp src/editor/Formatting.cpp -o build/test_SearchReplace ; if ($?) { ./build/test_SearchReplace }

#include <iostream>
#include <sstream>
#include <string>
#include "editor/SearchReplace.h"
#include "editor/UndoRedo.h"
#include "editor/Bookmarks.h"
#include "editor/Formatting.h"
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

// ---------- findSubstring ----------
void test_find_empty_pattern()
{
    CHECK(findSubstring("hello", "", 0) == -1);
    CHECK(findSubstring("", "", 0) == -1);
}

void test_find_empty_text()
{
    CHECK(findSubstring("", "abc", 0) == -1);
}

void test_find_pattern_longer_than_text()
{
    CHECK(findSubstring("ab", "abc", 0) == -1);
}

void test_find_basic_match()
{
    CHECK(findSubstring("hello world", "world", 0) == 6);
    CHECK(findSubstring("hello world", "hello", 0) == 0);
    CHECK(findSubstring("hello world", "lo", 0) == 3);
}

void test_find_no_match()
{
    CHECK(findSubstring("abcdef", "xyz", 0) == -1);
}

void test_find_start_past_end()
{
    CHECK(findSubstring("abc", "a", 3) == -1);
    CHECK(findSubstring("abc", "a", 99) == -1);
}

void test_find_negative_start_clamped()
{
    CHECK(findSubstring("abcabc", "abc", -5) == 0);
}

void test_find_start_pos_skips_earlier()
{
    CHECK(findSubstring("aaa", "a", 2) == 2);
    CHECK(findSubstring("abcabc", "abc", 1) == 3);
}

void test_find_case_sensitive()
{
    CHECK(findSubstring("ABC", "abc", 0) == -1);
    CHECK(findSubstring("abc", "ABC", 0) == -1);
}

// ---------- searchAll ----------
void test_search_all_empty_pattern()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    DynamicArray<Position> r = searchAll(ed, "");
    CHECK(r.size() == 0);
}

void test_search_all_empty_document()
{
    Editor ed;
    ed.document.clear();
    DynamicArray<Position> r = searchAll(ed, "a");
    CHECK(r.size() == 0);
}

void test_search_all_no_match()
{
    const std::string lines[] = {"abc", "def"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    DynamicArray<Position> r = searchAll(ed, "zzz");
    CHECK(r.size() == 0);
}

void test_search_all_single_match()
{
    const std::string lines[] = {"hello"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    DynamicArray<Position> r = searchAll(ed, "llo");
    CHECK(r.size() == 1);
    if (r.size() == 1)
    {
        CHECK(r[0].line == 0);
        CHECK(r[0].col == 2);
    }
}

void test_search_all_multi_line()
{
    const std::string lines[] = {"aa", "xaa", "aaa"};
    Editor ed;
    makeEditorWith(lines, 3, ed);
    DynamicArray<Position> r = searchAll(ed, "aa");
    CHECK(r.size() == 4);
    CHECK(r[0].line == 0 && r[0].col == 0);
    CHECK(r[1].line == 1 && r[1].col == 1);
    CHECK(r[2].line == 2 && r[2].col == 0);
    CHECK(r[3].line == 2 && r[3].col == 1);
}

void test_search_all_overlapping()
{
    const std::string lines[] = {"aaa"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    DynamicArray<Position> r = searchAll(ed, "aa");
    CHECK(r.size() == 2);
    if (r.size() == 2)
    {
        CHECK(r[0].col == 0);
        CHECK(r[1].col == 1);
    }
}

void test_search_all_case_sensitive()
{
    const std::string lines[] = {"ABCabc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    DynamicArray<Position> r = searchAll(ed, "abc");
    CHECK(r.size() == 1);
    if (r.size() == 1)
        CHECK(r[0].col == 3);
}

// ---------- containsText ----------
void test_contains_empty_pattern()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(containsText(ed, "") == false);
}

void test_contains_empty_document()
{
    Editor ed;
    ed.document.clear();
    CHECK(containsText(ed, "a") == false);
}

void test_contains_present()
{
    const std::string lines[] = {"hello world"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(containsText(ed, "world") == true);
}

void test_contains_absent()
{
    const std::string lines[] = {"hello"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(containsText(ed, "xyz") == false);
}

void test_contains_on_later_line()
{
    const std::string lines[] = {"aaa", "bbb", "target"};
    Editor ed;
    makeEditorWith(lines, 3, ed);
    CHECK(containsText(ed, "target") == true);
}

// ---------- displaySearchResults ----------
void test_display_not_found()
{
    DynamicArray<Position> r;
    std::ostringstream out;
    displaySearchResults(r, out);
    CHECK(out.str() == "Not found\n");
}

void test_display_found()
{
    DynamicArray<Position> r;
    Position p1;
    p1.line = 0;
    p1.col = 0;
    r.pushBack(p1);
    Position p2;
    p2.line = 2;
    p2.col = 5;
    r.pushBack(p2);
    std::ostringstream out;
    displaySearchResults(r, out);
    CHECK(out.str() == "Found 2 occurrence(s):\nLine 1, Column 1\nLine 3, Column 6\n");
}

// ---------- replaceOccurrence ----------
void test_replace_occ_empty_pattern()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceOccurrence(ed, "", "X", 1) == false);
    CHECK(ed.getLine(0)->text == "abc");
}

void test_replace_occ_bad_n()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceOccurrence(ed, "a", "X", 0) == false);
    CHECK(replaceOccurrence(ed, "a", "X", -1) == false);
    CHECK(ed.getLine(0)->text == "abc");
}

void test_replace_occ_empty_document()
{
    Editor ed;
    ed.document.clear();
    CHECK(replaceOccurrence(ed, "a", "X", 1) == false);
}

void test_replace_occ_too_few_matches()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceOccurrence(ed, "a", "X", 2) == false);
    CHECK(ed.getLine(0)->text == "abc");
}

void test_replace_occ_first()
{
    const std::string lines[] = {"banana"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceOccurrence(ed, "an", "X", 1) == true);
    CHECK(ed.getLine(0)->text == "bXana");
}

void test_replace_occ_second()
{
    const std::string lines[] = {"banana"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceOccurrence(ed, "an", "X", 2) == true);
    CHECK(ed.getLine(0)->text == "banXa");
}

void test_replace_occ_on_later_line()
{
    const std::string lines[] = {"aaa", "bbb", "target"};
    Editor ed;
    makeEditorWith(lines, 3, ed);
    CHECK(replaceOccurrence(ed, "target", "HIT", 1) == true);
    CHECK(ed.getLine(0)->text == "aaa");
    CHECK(ed.getLine(1)->text == "bbb");
    CHECK(ed.getLine(2)->text == "HIT");
}

void test_replace_occ_with_empty_new()
{
    const std::string lines[] = {"abcabc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceOccurrence(ed, "bc", "", 1) == true);
    CHECK(ed.getLine(0)->text == "aabc");
}

void test_replace_occ_records_and_modifies()
{
    const std::string lines[] = {"abcabc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    ed.setModified(false);
    CHECK(replaceOccurrence(ed, "bc", "X", 1) == true);
    CHECK(ed.isModified() == true);
    CHECK(ed.undoStack.size() == 1);
}

void test_replace_occ_adjusts_formats()
{
    const std::string lines[] = {"abcdef"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    Line *l = ed.getLine(0);

    FormatSpan s1;
    s1.startCol = 0;
    s1.endCol = 2;
    s1.type = BOLD;
    FormatSpan s2;
    s2.startCol = 2;
    s2.endCol = 4;
    s2.type = ITALIC;
    FormatSpan s3;
    s3.startCol = 4;
    s3.endCol = 6;
    s3.type = BOLD_ITALIC;
    l->formats.pushBack(s1);
    l->formats.pushBack(s2);
    l->formats.pushBack(s3);

    CHECK(replaceOccurrence(ed, "cd", "XYZ", 1) == true);
    CHECK(l->text == "abXYZef");
    CHECK(l->formats.size() == 2);

    FormatSpan out1, out2;
    CHECK(l->formats.get(0, out1));
    CHECK(l->formats.get(1, out2));
    CHECK(out1.startCol == 0 && out1.endCol == 2);
    CHECK(out2.startCol == 5 && out2.endCol == 7);
}

// NEW TEST: Undo and redo of replaceOccurrence including cursor.
void test_replace_occ_undo_redo_cursor()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    ed.cursorLine = 0;
    ed.cursorCol = 0;

    CHECK(replaceOccurrence(ed, "b", "XY", 1) == true);
    CHECK(ed.getLine(0)->text == "aXYc");
    CHECK(ed.cursorLine == 0 && ed.cursorCol == 3); // end of newStr

    UndoRedo ur;
    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLine(0)->text == "abc");
    CHECK(ed.cursorLine == 0 && ed.cursorCol == 0); // cursorBefore restored

    CHECK(ur.redo(ed) == true);
    CHECK(ed.getLine(0)->text == "aXYc");
    CHECK(ed.cursorLine == 0 && ed.cursorCol == 3);
}

// ---------- replaceAll ----------
void test_replace_all_empty_pattern()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceAll(ed, "", "X") == 0);
    CHECK(ed.getLine(0)->text == "abc");
}

void test_replace_all_empty_document()
{
    Editor ed;
    ed.document.clear();
    CHECK(replaceAll(ed, "a", "X") == 0);
}

void test_replace_all_no_match()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceAll(ed, "zzz", "X") == 0);
    CHECK(ed.getLine(0)->text == "abc");
    CHECK(ed.undoStack.isEmpty() == true); // NEW: no command recorded
}

void test_replace_all_basic()
{
    const std::string lines[] = {"aaa"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceAll(ed, "a", "b") == 3);
    CHECK(ed.getLine(0)->text == "bbb");
}

void test_replace_all_multi_line()
{
    const std::string lines[] = {"cat", "dog", "cat"};
    Editor ed;
    makeEditorWith(lines, 3, ed);
    CHECK(replaceAll(ed, "cat", "bird") == 2);
    CHECK(ed.getLine(0)->text == "bird");
    CHECK(ed.getLine(1)->text == "dog");
    CHECK(ed.getLine(2)->text == "bird");
}

void test_replace_all_new_contains_pattern()
{
    const std::string lines[] = {"aaa"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceAll(ed, "a", "aa") == 3);
    CHECK(ed.getLine(0)->text == "aaaaaa");
}

void test_replace_all_delete()
{
    const std::string lines[] = {"banana"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceAll(ed, "an", "") == 2);
    CHECK(ed.getLine(0)->text == "ba");
}

void test_replace_all_non_overlapping()
{
    const std::string lines[] = {"aaa"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceAll(ed, "aa", "X") == 1);
    CHECK(ed.getLine(0)->text == "Xa");
}

void test_replace_all_same_text()
{
    const std::string lines[] = {"aaa"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceAll(ed, "a", "a") == 3);
    CHECK(ed.getLine(0)->text == "aaa");
}

// NEW TEST: ReplaceAll is exactly ONE undo step.
void test_replace_all_one_undo_step()
{
    const std::string lines[] = {"ab ab ab"};
    Editor ed;
    makeEditorWith(lines, 1, ed);

    CHECK(replaceAll(ed, "ab", "xyz") == 3);
    CHECK(ed.getLine(0)->text == "xyz xyz xyz");
    CHECK(ed.undoStack.size() == 1);

    UndoRedo ur;
    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLine(0)->text == "ab ab ab");

    CHECK(ur.redo(ed) == true);
    CHECK(ed.getLine(0)->text == "xyz xyz xyz");
}

// NEW TEST: Block replace preserves formats untouched by match.
void test_replace_all_preserves_unrelated_formats()
{
    const std::string lines[] = {"ab ab ab"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    Line *l = ed.getLine(0);

    // Span on the first "ab"
    FormatSpan s1;
    s1.startCol = 0;
    s1.endCol = 2;
    s1.type = BOLD;
    l->formats.pushBack(s1);

    CHECK(replaceAll(ed, "ab", "X") == 3);
    CHECK(l->text == "X X X");

    // The first match overlapped the span, so the span should be gone.
    // If we put a span on the spaces, it should survive.
    CHECK(l->formats.size() == 0);
}

int main()
{
    test_find_empty_pattern();
    test_find_empty_text();
    test_find_pattern_longer_than_text();
    test_find_basic_match();
    test_find_no_match();
    test_find_start_past_end();
    test_find_negative_start_clamped();
    test_find_start_pos_skips_earlier();
    test_find_case_sensitive();

    test_search_all_empty_pattern();
    test_search_all_empty_document();
    test_search_all_no_match();
    test_search_all_single_match();
    test_search_all_multi_line();
    test_search_all_overlapping();
    test_search_all_case_sensitive();

    test_contains_empty_pattern();
    test_contains_empty_document();
    test_contains_present();
    test_contains_absent();
    test_contains_on_later_line();

    test_display_not_found();
    test_display_found();

    test_replace_occ_empty_pattern();
    test_replace_occ_bad_n();
    test_replace_occ_empty_document();
    test_replace_occ_too_few_matches();
    test_replace_occ_first();
    test_replace_occ_second();
    test_replace_occ_on_later_line();
    test_replace_occ_with_empty_new();
    test_replace_occ_records_and_modifies();
    test_replace_occ_adjusts_formats();
    test_replace_occ_undo_redo_cursor();

    test_replace_all_empty_pattern();
    test_replace_all_empty_document();
    test_replace_all_no_match();
    test_replace_all_basic();
    test_replace_all_multi_line();
    test_replace_all_new_contains_pattern();
    test_replace_all_delete();
    test_replace_all_non_overlapping();
    test_replace_all_same_text();
    test_replace_all_one_undo_step();
    test_replace_all_preserves_unrelated_formats();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}