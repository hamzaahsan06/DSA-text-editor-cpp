// g++ -std=c++17 -Wall -Wextra -Iinclude -Iinclude/editor tests/editor/test_SearchReplace.cpp src/editor/SearchReplace.cpp src/editor/UndoRedo.cpp -o build/test_SearchReplace ; if ($?) { ./build/test_SearchReplace }

#include <iostream>
#include <sstream>
#include <string>
#include "editor/SearchReplace.h"
#include "editor/UndoRedo.h"
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

// Empty pattern returns -1.
void test_find_empty_pattern()
{
    CHECK(findSubstring("hello", "", 0) == -1);
    CHECK(findSubstring("", "", 0) == -1);
}

// Empty text with a non-empty pattern returns -1.
void test_find_empty_text()
{
    CHECK(findSubstring("", "abc", 0) == -1);
}

// Pattern longer than text returns -1.
void test_find_pattern_longer_than_text()
{
    CHECK(findSubstring("ab", "abc", 0) == -1);
}

// Basic match returns the index.
void test_find_basic_match()
{
    CHECK(findSubstring("hello world", "world", 0) == 6);
    CHECK(findSubstring("hello world", "hello", 0) == 0);
    CHECK(findSubstring("hello world", "lo", 0) == 3);
}

// No match returns -1.
void test_find_no_match()
{
    CHECK(findSubstring("abcdef", "xyz", 0) == -1);
}

// startPos past end returns -1.
void test_find_start_past_end()
{
    CHECK(findSubstring("abc", "a", 3) == -1);
    CHECK(findSubstring("abc", "a", 99) == -1);
}

// Negative startPos is clamped to 0.
void test_find_negative_start_clamped()
{
    CHECK(findSubstring("abcabc", "abc", -5) == 0);
}

// Skips matches before startPos.
void test_find_start_pos_skips_earlier()
{
    CHECK(findSubstring("aaa", "a", 2) == 2);
    CHECK(findSubstring("abcabc", "abc", 1) == 3);
}

// Case sensitivity: "ABC" not found by "abc".
void test_find_case_sensitive()
{
    CHECK(findSubstring("ABC", "abc", 0) == -1);
    CHECK(findSubstring("abc", "ABC", 0) == -1);
}

// ---------- searchAll ----------

// Empty pattern yields empty results.
void test_search_all_empty_pattern()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    DynamicArray<Position> r = searchAll(ed, "");
    CHECK(r.size() == 0);
}

// Empty document (zero lines) yields empty results.
void test_search_all_empty_document()
{
    Editor ed;
    ed.document.clear();
    DynamicArray<Position> r = searchAll(ed, "a");
    CHECK(r.size() == 0);
}

// No matches yields empty results.
void test_search_all_no_match()
{
    const std::string lines[] = {"abc", "def"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    DynamicArray<Position> r = searchAll(ed, "zzz");
    CHECK(r.size() == 0);
}

// Single match reports the right line and column.
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

// Matches are collected line-by-line across multiple lines.
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

// Overlapping matches are counted ("aa" in "aaa" -> 2).
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

// Case-sensitive search.
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

// Empty pattern returns false.
void test_contains_empty_pattern()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(containsText(ed, "") == false);
}

// Empty document returns false.
void test_contains_empty_document()
{
    Editor ed;
    ed.document.clear();
    CHECK(containsText(ed, "a") == false);
}

// Match present returns true.
void test_contains_present()
{
    const std::string lines[] = {"hello world"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(containsText(ed, "world") == true);
}

// Absent pattern returns false.
void test_contains_absent()
{
    const std::string lines[] = {"hello"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(containsText(ed, "xyz") == false);
}

// Match on a later line returns true.
void test_contains_on_later_line()
{
    const std::string lines[] = {"aaa", "bbb", "target"};
    Editor ed;
    makeEditorWith(lines, 3, ed);
    CHECK(containsText(ed, "target") == true);
}

// ---------- displaySearchResults ----------

// Empty results prints "Not found".
void test_display_not_found()
{
    DynamicArray<Position> r;
    std::ostringstream out;
    displaySearchResults(r, out);
    CHECK(out.str() == "Not found\n");
}

// Non-empty results prints count and 1-based positions.
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
    CHECK(out.str() ==
          "Found 2 occurrence(s):\n"
          "Line 1, Column 1\n"
          "Line 3, Column 6\n");
}

// ---------- replaceOccurrence ----------

// Empty pattern returns false.
void test_replace_occ_empty_pattern()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceOccurrence(ed, "", "X", 1) == false);
    CHECK(ed.getLine(0)->text == "abc");
}

// n < 1 returns false.
void test_replace_occ_bad_n()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceOccurrence(ed, "a", "X", 0) == false);
    CHECK(replaceOccurrence(ed, "a", "X", -1) == false);
    CHECK(ed.getLine(0)->text == "abc");
}

// Empty document returns false.
void test_replace_occ_empty_document()
{
    Editor ed;
    ed.document.clear();
    CHECK(replaceOccurrence(ed, "a", "X", 1) == false);
}

// Fewer matches than n returns false and does not modify.
void test_replace_occ_too_few_matches()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceOccurrence(ed, "a", "X", 2) == false);
    CHECK(ed.getLine(0)->text == "abc");
}

// Replaces the first match.
void test_replace_occ_first()
{
    const std::string lines[] = {"banana"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceOccurrence(ed, "an", "X", 1) == true);
    CHECK(ed.getLine(0)->text == "bXana");
}

// Replaces the second match (overlapping-search semantics).
void test_replace_occ_second()
{
    const std::string lines[] = {"banana"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceOccurrence(ed, "an", "X", 2) == true);
    CHECK(ed.getLine(0)->text == "banXa");
}

// Replaces a match on a later line.
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

// Empty newStr deletes the match.
void test_replace_occ_with_empty_new()
{
    const std::string lines[] = {"abcabc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceOccurrence(ed, "bc", "", 1) == true);
    CHECK(ed.getLine(0)->text == "aabc");
}

// Sets modified flag and records one REPLACE command.
void test_replace_occ_records_and_modifies()
{
    const std::string lines[] = {"abcabc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    ed.setModified(false);
    CHECK(replaceOccurrence(ed, "bc", "X", 1) == true);
    CHECK(ed.isModified() == true);
    CHECK(ed.undoStack.size() == 1);
    Command c;
    CHECK(ed.undoStack.peek(c) == true);
    CHECK(c.type == REPLACE);
    CHECK(c.line == 0);
    CHECK(c.col == 1);
    CHECK(c.oldText == "bc");
    CHECK(c.newText == "X");
}

// Undo after replaceOccurrence restores the original text.
void test_replace_occ_undo_restores()
{
    const std::string lines[] = {"banana"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    UndoRedo ur;
    CHECK(replaceOccurrence(ed, "an", "X", 2) == true);
    CHECK(ed.getLine(0)->text == "banXa");
    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLine(0)->text == "banana");
}

// Replaces formats that overlap the edit are dropped; later formats shift.
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

    // Replace "cd" (cols 2..4) with "XYZ" (length 3) -> delta +1.
    CHECK(replaceOccurrence(ed, "cd", "XYZ", 1) == true);
    CHECK(l->text == "abXYZef");
    CHECK(l->formats.size() == 2);
    FormatSpan out1, out2;
    CHECK(l->formats.get(0, out1));
    CHECK(l->formats.get(1, out2));
    CHECK(out1.startCol == 0 && out1.endCol == 2);
    CHECK(out2.startCol == 5 && out2.endCol == 7);
}

// ---------- replaceAll ----------

// Empty pattern returns 0 and does not modify.
void test_replace_all_empty_pattern()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceAll(ed, "", "X") == 0);
    CHECK(ed.getLine(0)->text == "abc");
}

// Empty document returns 0.
void test_replace_all_empty_document()
{
    Editor ed;
    ed.document.clear();
    CHECK(replaceAll(ed, "a", "X") == 0);
}

// No matches returns 0 and does not modify.
void test_replace_all_no_match()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceAll(ed, "zzz", "X") == 0);
    CHECK(ed.getLine(0)->text == "abc");
}

// Replaces every non-overlapping occurrence.
void test_replace_all_basic()
{
    const std::string lines[] = {"aaa"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceAll(ed, "a", "b") == 3);
    CHECK(ed.getLine(0)->text == "bbb");
}

// Replaces matches on multiple lines.
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

// newStr containing pattern must not loop forever ("a" -> "aa").
void test_replace_all_new_contains_pattern()
{
    const std::string lines[] = {"aaa"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceAll(ed, "a", "aa") == 3);
    CHECK(ed.getLine(0)->text == "aaaaaa");
}

// Empty newStr deletes every match.
void test_replace_all_delete()
{
    const std::string lines[] = {"banana"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceAll(ed, "an", "") == 2);
    CHECK(ed.getLine(0)->text == "ba");
}

// Records one REPLACE command per replacement and sets modified.
void test_replace_all_records_and_modifies()
{
    const std::string lines[] = {"aaa"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    ed.setModified(false);
    CHECK(replaceAll(ed, "a", "b") == 3);
    CHECK(ed.isModified() == true);
    CHECK(ed.undoStack.size() == 3);
}

// Undo restores the original text after replaceAll (one undo per replacement).
void test_replace_all_undo_each()
{
    const std::string lines[] = {"aaa"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    UndoRedo ur;
    CHECK(replaceAll(ed, "a", "b") == 3);
    CHECK(ed.getLine(0)->text == "bbb");
    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLine(0)->text == "bba");
    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLine(0)->text == "baa");
    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLine(0)->text == "aaa");
}

// Overlapping: replaceAll is non-overlapping ("aa" in "aaa" -> 1 replacement).
void test_replace_all_non_overlapping()
{
    const std::string lines[] = {"aaa"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceAll(ed, "aa", "X") == 1);
    CHECK(ed.getLine(0)->text == "Xa");
}

// Pattern equal to newStr still returns the count of matches.
void test_replace_all_same_text()
{
    const std::string lines[] = {"aaa"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(replaceAll(ed, "a", "a") == 3);
    CHECK(ed.getLine(0)->text == "aaa");
}

// replaceAll grows a line and drops spans overlapped by the whole-line edit.
void test_replace_all_adjusts_formats()
{
    const std::string lines[] = {"abab"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    Line *l = ed.getLine(0);

    FormatSpan s;
    s.startCol = 0;
    s.endCol = 4;
    s.type = BOLD;
    l->formats.pushBack(s);

    CHECK(replaceAll(ed, "ab", "X") == 2);
    CHECK(l->text == "XX");
    CHECK(l->formats.size() == 0);
}

// ---------- format-span handling in replaceAll ----------

// replaceAll drops every format span on a line it rewrites, because it
// applies a whole-line edit range [0, source.length()) to the span adjuster.
void test_replace_all_drops_all_spans_on_modified_line()
{
    const std::string lines[] = {"aXbXc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    Line *l = ed.getLine(0);

    // Three spans: before, between, after the two matches.
    FormatSpan s1;
    s1.startCol = 0;
    s1.endCol = 1;
    s1.type = BOLD;
    FormatSpan s2;
    s2.startCol = 2;
    s2.endCol = 3;
    s2.type = ITALIC;
    FormatSpan s3;
    s3.startCol = 4;
    s3.endCol = 5;
    s3.type = BOLD_ITALIC;
    l->formats.pushBack(s1);
    l->formats.pushBack(s2);
    l->formats.pushBack(s3);

    // Replace "X" -> "YY": matches at 1 and 3, result "aYYbYYc".
    CHECK(replaceAll(ed, "X", "YY") == 2);
    CHECK(l->text == "aYYbYYc");
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
    test_replace_occ_undo_restores();
    test_replace_occ_adjusts_formats();

    test_replace_all_empty_pattern();
    test_replace_all_empty_document();
    test_replace_all_no_match();
    test_replace_all_basic();
    test_replace_all_multi_line();
    test_replace_all_new_contains_pattern();
    test_replace_all_delete();
    test_replace_all_records_and_modifies();
    test_replace_all_undo_each();
    test_replace_all_non_overlapping();
    test_replace_all_same_text();
    test_replace_all_adjusts_formats();
    test_replace_all_drops_all_spans_on_modified_line();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}