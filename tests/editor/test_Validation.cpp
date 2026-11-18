// g++ -std=c++17 -Wall -Wextra -Iinclude -Iinclude/editor tests/editor/test_Validation.cpp src/editor/Validation.cpp -o build/test_Validation ; if ($?) { ./build/test_Validation }

#include <iostream>
#include <string>
#include "editor/Validation.h"
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

// ---------- isDocumentEmpty ----------

// A freshly reset editor has one empty line, so it is not empty.
void test_is_document_empty_false_on_reset()
{
    Editor ed;
    CHECK(isDocumentEmpty(ed) == false);
}

// A cleared document is empty.
void test_is_document_empty_true_when_cleared()
{
    Editor ed;
    ed.document.clear();
    CHECK(isDocumentEmpty(ed) == true);
}

// ---------- isEmptyString ----------

// An empty std::string is empty.
void test_is_empty_string_true()
{
    CHECK(isEmptyString("") == true);
    std::string s;
    CHECK(isEmptyString(s) == true);
}

// A non-empty std::string is not empty.
void test_is_empty_string_false()
{
    CHECK(isEmptyString("a") == false);
    CHECK(isEmptyString(" ") == false);
    CHECK(isEmptyString("hello") == false);
}

// ---------- isValidLine ----------

// A line at index 0 of a non-empty document is valid.
void test_is_valid_line_first()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(isValidLine(ed, 0) == true);
}

// The last line of the document is valid.
void test_is_valid_line_last()
{
    const std::string lines[] = {"a", "b", "c"};
    Editor ed;
    makeEditorWith(lines, 3, ed);
    CHECK(isValidLine(ed, 2) == true);
}

// Out-of-range lines are rejected.
void test_is_valid_line_out_of_range()
{
    const std::string lines[] = {"a", "b"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    CHECK(isValidLine(ed, -1) == false);
    CHECK(isValidLine(ed, 2) == false);
    CHECK(isValidLine(ed, 99) == false);
}

// A cleared document rejects even line 0.
void test_is_valid_line_empty_document()
{
    Editor ed;
    ed.document.clear();
    CHECK(isValidLine(ed, 0) == false);
}

// ---------- isValidColumn ----------

// Column 0 is valid for any existing line.
void test_is_valid_column_zero()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(isValidColumn(ed, 0, 0) == true);
}

// Column == length is valid (end of line).
void test_is_valid_column_end_of_line()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(isValidColumn(ed, 0, 3) == true);
}

// A column beyond the line length is rejected.
void test_is_valid_column_past_end()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(isValidColumn(ed, 0, 4) == false);
    CHECK(isValidColumn(ed, 0, 99) == false);
}

// Negative columns are rejected.
void test_is_valid_column_negative()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(isValidColumn(ed, 0, -1) == false);
}

// An invalid line makes isValidColumn return false regardless of col.
void test_is_valid_column_bad_line()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(isValidColumn(ed, 5, 0) == false);
    CHECK(isValidColumn(ed, -1, 0) == false);
}

// Empty line: only col 0 is valid.
void test_is_valid_column_empty_line()
{
    const std::string lines[] = {""};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(isValidColumn(ed, 0, 0) == true);
    CHECK(isValidColumn(ed, 0, 1) == false);
}

// Works on a line reached from the tail side (index >= n/2).
void test_is_valid_column_later_line()
{
    const std::string lines[] = {"aa", "bbb", "cccc", "ddddd"};
    Editor ed;
    makeEditorWith(lines, 4, ed);
    CHECK(isValidColumn(ed, 3, 5) == true); // length of "ddddd"
    CHECK(isValidColumn(ed, 3, 6) == false);
    CHECK(isValidColumn(ed, 2, 4) == true); // length of "cccc"
    CHECK(isValidColumn(ed, 2, 5) == false);
}

// ---------- isValidRange ----------

// A single-line range is valid.
void test_is_valid_range_single()
{
    const std::string lines[] = {"a", "b", "c"};
    Editor ed;
    makeEditorWith(lines, 3, ed);
    CHECK(isValidRange(ed, 1, 1) == true);
}

// A multi-line range with start <= end is valid.
void test_is_valid_range_multi()
{
    const std::string lines[] = {"a", "b", "c", "d"};
    Editor ed;
    makeEditorWith(lines, 4, ed);
    CHECK(isValidRange(ed, 0, 3) == true);
    CHECK(isValidRange(ed, 1, 2) == true);
}

// start > end is invalid.
void test_is_valid_range_inverted()
{
    const std::string lines[] = {"a", "b", "c"};
    Editor ed;
    makeEditorWith(lines, 3, ed);
    CHECK(isValidRange(ed, 2, 1) == false);
    CHECK(isValidRange(ed, 1, 0) == false);
}

// A range with a bad endpoint is invalid.
void test_is_valid_range_bad_endpoint()
{
    const std::string lines[] = {"a", "b"};
    Editor ed;
    makeEditorWith(lines, 2, ed);
    CHECK(isValidRange(ed, -1, 0) == false);
    CHECK(isValidRange(ed, 0, 5) == false);
    CHECK(isValidRange(ed, 5, 6) == false);
}

// ---------- isValidColumnRange ----------

// A valid range within a line's length is accepted.
void test_is_valid_column_range_valid()
{
    const std::string lines[] = {"abcdef"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(isValidColumnRange(ed, 0, 0, 6) == true);
    CHECK(isValidColumnRange(ed, 0, 2, 4) == true);
}

// An empty range (start == end) is valid here; caller can reject it.
void test_is_valid_column_range_empty()
{
    const std::string lines[] = {"abcdef"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(isValidColumnRange(ed, 0, 3, 3) == true);
}

// start > end is invalid.
void test_is_valid_column_range_inverted()
{
    const std::string lines[] = {"abcdef"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(isValidColumnRange(ed, 0, 4, 2) == false);
}

// startCol < 0 is invalid.
void test_is_valid_column_range_negative_start()
{
    const std::string lines[] = {"abcdef"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(isValidColumnRange(ed, 0, -1, 3) == false);
}

// endCol > length is invalid.
void test_is_valid_column_range_end_past_eol()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(isValidColumnRange(ed, 0, 0, 4) == false);
    CHECK(isValidColumnRange(ed, 0, 1, 99) == false);
}

// A bad line makes isValidColumnRange return false.
void test_is_valid_column_range_bad_line()
{
    const std::string lines[] = {"abc"};
    Editor ed;
    makeEditorWith(lines, 1, ed);
    CHECK(isValidColumnRange(ed, 5, 0, 3) == false);
    CHECK(isValidColumnRange(ed, -1, 0, 3) == false);
}

// Works on a later line reached from the tail.
void test_is_valid_column_range_later_line()
{
    const std::string lines[] = {"x", "y", "zzzz", "wwwwww"};
    Editor ed;
    makeEditorWith(lines, 4, ed);
    CHECK(isValidColumnRange(ed, 3, 0, 6) == true); // "wwwwww"
    CHECK(isValidColumnRange(ed, 3, 0, 7) == false);
    CHECK(isValidColumnRange(ed, 2, 1, 4) == true); // "zzzz"
}

int main()
{
    test_is_document_empty_false_on_reset();
    test_is_document_empty_true_when_cleared();

    test_is_empty_string_true();
    test_is_empty_string_false();

    test_is_valid_line_first();
    test_is_valid_line_last();
    test_is_valid_line_out_of_range();
    test_is_valid_line_empty_document();

    test_is_valid_column_zero();
    test_is_valid_column_end_of_line();
    test_is_valid_column_past_end();
    test_is_valid_column_negative();
    test_is_valid_column_bad_line();
    test_is_valid_column_empty_line();
    test_is_valid_column_later_line();

    test_is_valid_range_single();
    test_is_valid_range_multi();
    test_is_valid_range_inverted();
    test_is_valid_range_bad_endpoint();

    test_is_valid_column_range_valid();
    test_is_valid_column_range_empty();
    test_is_valid_column_range_inverted();
    test_is_valid_column_range_negative_start();
    test_is_valid_column_range_end_past_eol();
    test_is_valid_column_range_bad_line();
    test_is_valid_column_range_later_line();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}