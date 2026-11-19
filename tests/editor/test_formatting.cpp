// g++ -std=c++11 -Wall -Wextra -Iinclude -Iinclude/editor tests/editor/test_formatting.cpp src/editor/Formatting.cpp -o build/test_formatting ; if ($?) { ./build/test_formatting }

#include <iostream>
#include <string>
#include "editor/Formatting.h"
#include "editor/Types.h"
#include "../ds/check.h"

// ---------- addFormat ----------

// Adding a BOLD span succeeds.
void test_add_bold_span()
{
    Line l; l.text = "hello";
    CHECK(addFormat(l, 0, 2, true, false) == true);
    CHECK(l.formats.size() == 1);
    FormatSpan s; l.formats.get(0, s);
    CHECK(s.startCol == 0 && s.endCol == 2 && s.type == BOLD);
}

// Adding an ITALIC span succeeds.
void test_add_italic_span()
{
    Line l; l.text = "hello";
    CHECK(addFormat(l, 1, 4, false, true) == true);
    CHECK(l.formats.size() == 1);
    FormatSpan s; l.formats.get(0, s);
    CHECK(s.type == ITALIC);
}

// Adding a BOLD_ITALIC span succeeds.
void test_add_bold_italic_span()
{
    Line l; l.text = "hello";
    CHECK(addFormat(l, 0, 5, true, true) == true);
    CHECK(l.formats.size() == 1);
    FormatSpan s; l.formats.get(0, s);
    CHECK(s.type == BOLD_ITALIC);
}

// Rejects negative startCol.
void test_add_negative_start_rejected()
{
    Line l; l.text = "hello";
    CHECK(addFormat(l, -1, 2, true, false) == false);
    CHECK(l.formats.size() == 0);
}

// Rejects startCol >= endCol (inverted or zero-length range).
void test_add_inverted_or_zero_range_rejected()
{
    Line l; l.text = "hello";
    CHECK(addFormat(l, 2, 2, true, false) == false);
    CHECK(addFormat(l, 3, 1, true, false) == false);
    CHECK(l.formats.size() == 0);
}

// Rejects endCol > text length.
void test_add_end_past_text_rejected()
{
    Line l; l.text = "abc";
    CHECK(addFormat(l, 0, 4, true, false) == false);
    CHECK(l.formats.size() == 0);
}

// Rejects no-bold-no-italic.
void test_add_plain_span_rejected()
{
    Line l; l.text = "hello";
    CHECK(addFormat(l, 0, 2, false, false) == false);
    CHECK(l.formats.size() == 0);
}

// endCol == text.length() is accepted.
void test_add_end_equals_length_accepted()
{
    Line l; l.text = "abc";
    CHECK(addFormat(l, 0, 3, true, false) == true);
    CHECK(l.formats.size() == 1);
}

// ---------- removeFormat ----------

// Removes a span at a valid index.
void test_remove_valid_index()
{
    Line l; l.text = "hello";
    addFormat(l, 0, 2, true, false);
    addFormat(l, 2, 4, false, true);
    CHECK(l.formats.size() == 2);
    CHECK(removeFormat(l, 0) == true);
    CHECK(l.formats.size() == 1);
}

// Rejects bad indices.
void test_remove_bad_index()
{
    Line l; l.text = "hello";
    addFormat(l, 0, 2, true, false);
    CHECK(removeFormat(l, -1) == false);
    CHECK(removeFormat(l, 1) == false);
    CHECK(removeFormat(l, 99) == false);
    CHECK(l.formats.size() == 1);
}

// ---------- clearFormats ----------

// Removes all spans.
void test_clear_formats()
{
    Line l; l.text = "hello";
    addFormat(l, 0, 2, true, false);
    addFormat(l, 2, 4, false, true);
    CHECK(l.formats.size() == 2);
    clearFormats(l);
    CHECK(l.formats.size() == 0);
}

// ---------- renderLine (ANSI) ----------

// Empty line renders as empty string.
void test_render_empty_line_ansi()
{
    Line l;
    CHECK(renderLine(l, true) == "");
}

// No spans returns the text unchanged.
void test_render_no_spans_ansi()
{
    Line l; l.text = "hello";
    CHECK(renderLine(l, true) == "hello");
}

// Span at start renders codes only around the covered part.
void test_render_span_at_start_ansi()
{
    Line l; l.text = "abcde";
    addFormat(l, 0, 2, true, false);
    CHECK(renderLine(l, true) == "\033[1mab\033[0m" "cde");
}

// Span at end renders codes only around the covered part.
void test_render_span_at_end_ansi()
{
    Line l; l.text = "abcde";
    addFormat(l, 3, 5, true, false);
    CHECK(renderLine(l, true) == "abc" "\033[1mde\033[0m");
}

// Adjacent spans of different styles change codes without redundant resets.
void test_render_adjacent_spans_ansi()
{
    Line l; l.text = "abcd";
    addFormat(l, 0, 2, true, false);  // Bold
    addFormat(l, 2, 4, false, true);  // Italic
    // Transition at index 2 should close bold and open italic.
    CHECK(renderLine(l, true) == "\033[1mab\033[0m" "\033[3mcd\033[0m");
}

// Overlapping spans combine: bold+italic overlap.
void test_render_overlapping_spans_ansi()
{
    Line l; l.text = "abcde";
    addFormat(l, 0, 3, true, false);  // Bold 0..2
    addFormat(l, 2, 5, false, true);  // Italic 2..4
    // Col 0-1: bold only, Col 2: bold+italic, Col 3-4: italic only
    CHECK(renderLine(l, true) == 
        "\033[1mab\033[0m" 
        "\033[1m\033[3mc\033[0m" 
        "\033[3mde\033[0m");
}

// A span reaching past the text length ignores the bad part.
void test_render_span_past_text_ansi()
{
    Line l; l.text = "abc";
    // Force a bad span (bypassing addFormat safety)
    FormatSpan s; s.startCol = 1; s.endCol = 99; s.type = BOLD;
    l.formats.pushBack(s);
    CHECK(renderLine(l, true) == "a\033[1mbc\033[0m");
}

// ---------- renderLine (Fallback) ----------

// Empty line renders as empty string.
void test_render_empty_line_fallback()
{
    Line l;
    CHECK(renderLine(l, false) == "");
}

// No spans returns the text unchanged.
void test_render_no_spans_fallback()
{
    Line l; l.text = "hello";
    CHECK(renderLine(l, false) == "hello");
}

// Bold renders as **text**.
void test_render_bold_fallback()
{
    Line l; l.text = "abc";
    addFormat(l, 0, 2, true, false);
    CHECK(renderLine(l, false) == "**ab**c");
}

// Italic renders as *text*.
void test_render_italic_fallback()
{
    Line l; l.text = "abc";
    addFormat(l, 1, 3, false, true);
    CHECK(renderLine(l, false) == "a*bc*");
}

// Bold+Italic renders as ***text***.
void test_render_bold_italic_fallback()
{
    Line l; l.text = "abcd";
    addFormat(l, 1, 3, true, true);
    CHECK(renderLine(l, false) == "a***bc***d");
}

// Adjacent different styles.
void test_render_adjacent_spans_fallback()
{
    Line l; l.text = "abcd";
    addFormat(l, 0, 2, true, false);  // **ab**
    addFormat(l, 2, 4, false, true);  // *cd*
    CHECK(renderLine(l, false) == "**ab***cd*");
}

int main()
{
    test_add_bold_span();
    test_add_italic_span();
    test_add_bold_italic_span();
    test_add_negative_start_rejected();
    test_add_inverted_or_zero_range_rejected();
    test_add_end_past_text_rejected();
    test_add_plain_span_rejected();
    test_add_end_equals_length_accepted();

    test_remove_valid_index();
    test_remove_bad_index();
    test_clear_formats();

    test_render_empty_line_ansi();
    test_render_no_spans_ansi();
    test_render_span_at_start_ansi();
    test_render_span_at_end_ansi();
    test_render_adjacent_spans_ansi();
    test_render_overlapping_spans_ansi();
    test_render_span_past_text_ansi();

    test_render_empty_line_fallback();
    test_render_no_spans_fallback();
    test_render_bold_fallback();
    test_render_italic_fallback();
    test_render_bold_italic_fallback();
    test_render_adjacent_spans_fallback();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}