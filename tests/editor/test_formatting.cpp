// g++ -std=c++17 -Wall -Wextra -Iinclude -Iinclude/editor tests/editor/test_Formatting.cpp src/editor/Formatting.cpp -o build/test_Formatting ; if ($?) { ./build/test_Formatting }

#include <iostream>
#include <string>
#include "editor/Formatting.h"
#include "../ds/check.h"

// ---------- addFormat ----------

// Rejects negative start, end < start, and no-bold-no-italic spans.
void test_add_format_rejects_bad_input()
{
    Formatting f;
    CHECK(f.addFormat(-1, 2, true, false) == false);
    CHECK(f.addFormat(2, 1, true, false) == false);
    CHECK(f.addFormat(1, 1, false, false) == false);
    CHECK(f.count() == 0);
}

// Adds a BOLD span: applying produces the bold escape around the covered chars.
void test_add_format_bold()
{
    Formatting f;
    CHECK(f.addFormat(0, 4, true, false) == true);
    CHECK(f.count() == 1);
    std::string got = f.applyFormatting("abc");
    std::string want =
        "\033[1m" "a" "\033[0m"
        "\033[1m" "b" "\033[0m"
        "\033[1m" "c" "\033[0m";
    CHECK(got == want);
}

// Adds an ITALIC span: applying produces the italic escape.
void test_add_format_italic()
{
    Formatting f;
    CHECK(f.addFormat(2, 5, false, true) == true);
    CHECK(f.count() == 1);
    // Line shorter than startCol: no effect.
    CHECK(f.applyFormatting("ab") == "ab");
    // Line that reaches startCol: italic kicks in at column 2 onward.
    std::string got = f.applyFormatting("abcd");
    std::string want =
        "ab"
        "\033[3m" "c" "\033[0m"
        "\033[3m" "d" "\033[0m";
    CHECK(got == want);
}

// Adds a BOLD_ITALIC span: applying produces both codes.
void test_add_format_bold_italic()
{
    Formatting f;
    CHECK(f.addFormat(1, 3, true, true) == true);
    CHECK(f.count() == 1);
    std::string got = f.applyFormatting("abcd");
    std::string want =
        "a"
        "\033[1m" "\033[3m" "b" "\033[0m"
        "\033[1m" "\033[3m" "c" "\033[0m"
        "\033[1m" "\033[3m" "d" "\033[0m";
    CHECK(got == want);
}

// Zero-length span (start == end) is accepted.
void test_add_format_zero_length_accepted()
{
    Formatting f;
    CHECK(f.addFormat(2, 2, true, false) == true);
    CHECK(f.count() == 1);
}

// ---------- removeFormat ----------

// Removes the span at a valid index; the surviving span still applies.
void test_remove_format_valid()
{
    Formatting f;
    f.addFormat(0, 1, true, false);    // bold on cols 0..1
    f.addFormat(2, 3, false, true);    // italic on cols 2..3
    CHECK(f.count() == 2);

    CHECK(f.removeFormat(0) == true);
    CHECK(f.count() == 1);

    // After removing the bold span, only italic remains on cols 2..3.
    std::string got = f.applyFormatting("abcd");
    std::string want =
        "ab"
        "\033[3m" "c" "\033[0m"
        "\033[3m" "d" "\033[0m";
    CHECK(got == want);
}

// Rejects out-of-range indexes (negative and >= count).
void test_remove_format_invalid()
{
    Formatting f;
    f.addFormat(0, 2, true, false);
    CHECK(f.removeFormat(-1) == false);
    CHECK(f.removeFormat(1) == false);
    CHECK(f.removeFormat(99) == false);
    CHECK(f.count() == 1);
}

// ---------- clearFormats / count ----------

// clearFormats empties the span list; applyFormatting then has no effect.
void test_clear_and_count()
{
    Formatting f;
    CHECK(f.count() == 0);
    f.addFormat(0, 1, true, false);
    f.addFormat(2, 3, false, true);
    CHECK(f.count() == 2);

    CHECK(f.applyFormatting("abcd") != "abcd");   // some formatting was applied

    f.clearFormats();
    CHECK(f.count() == 0);
    CHECK(f.applyFormatting("abcd") == "abcd");   // now no formatting
}

// ---------- applyFormatting ----------

// Empty line returns empty string regardless of spans.
void test_apply_empty_line()
{
    Formatting f;
    f.addFormat(0, 5, true, false);
    CHECK(f.applyFormatting("") == "");
}

// No spans returns the line unchanged (no ANSI codes).
void test_apply_no_spans()
{
    Formatting f;
    CHECK(f.applyFormatting("hello") == "hello");
}

// BOLD wraps only the covered characters with \033[1m ... \033[0m.
void test_apply_bold()
{
    Formatting f;
    f.addFormat(1, 3, true, false);
    std::string got = f.applyFormatting("abcde");
    std::string want =
        "a"
        "\033[1m" "b" "\033[0m"
        "\033[1m" "c" "\033[0m"
        "\033[1m" "d" "\033[0m"
        "e";
    CHECK(got == want);
}

// ITALIC wraps only the covered characters with \033[3m ... \033[0m.
void test_apply_italic()
{
    Formatting f;
    f.addFormat(0, 1, false, true);
    std::string got = f.applyFormatting("abc");
    std::string want =
        "\033[3m" "a" "\033[0m"
        "\033[3m" "b" "\033[0m"
        "c";
    CHECK(got == want);
}

// BOLD_ITALIC emits both codes in order (bold then italic).
void test_apply_bold_italic()
{
    Formatting f;
    f.addFormat(0, 0, true, true);
    std::string got = f.applyFormatting("x");
    std::string want = "\033[1m" "\033[3m" "x" "\033[0m";
    CHECK(got == want);
}

// Two overlapping spans combine: bold from one, italic from another.
void test_apply_overlapping_spans()
{
    Formatting f;
    f.addFormat(0, 2, true, false);
    f.addFormat(1, 3, false, true);
    std::string got = f.applyFormatting("abcd");
    std::string want =
        "\033[1m" "a" "\033[0m"
        "\033[1m" "\033[3m" "b" "\033[0m"
        "\033[1m" "\033[3m" "c" "\033[0m"
        "\033[3m" "d" "\033[0m";
    CHECK(got == want);
}

// endCol is inclusive: a span [1,1] touches only one character.
void test_apply_inclusive_end()
{
    Formatting f;
    f.addFormat(1, 1, true, false);
    std::string got = f.applyFormatting("abc");
    std::string want =
        "a"
        "\033[1m" "b" "\033[0m"
        "c";
    CHECK(got == want);
}

// A span that lies fully beyond the line length has no effect.
void test_apply_span_beyond_line()
{
    Formatting f;
    f.addFormat(10, 20, true, false);
    CHECK(f.applyFormatting("abc") == "abc");
}

// A span partially covering the line only affects the in-range characters.
void test_apply_span_partially_in_range()
{
    Formatting f;
    f.addFormat(1, 100, true, false);
    std::string got = f.applyFormatting("abc");
    std::string want =
        "a"
        "\033[1m" "b" "\033[0m"
        "\033[1m" "c" "\033[0m";
    CHECK(got == want);
}

int main()
{
    test_add_format_rejects_bad_input();
    test_add_format_bold();
    test_add_format_italic();
    test_add_format_bold_italic();
    test_add_format_zero_length_accepted();

    test_remove_format_valid();
    test_remove_format_invalid();

    test_clear_and_count();

    test_apply_empty_line();
    test_apply_no_spans();
    test_apply_bold();
    test_apply_italic();
    test_apply_bold_italic();
    test_apply_overlapping_spans();
    test_apply_inclusive_end();
    test_apply_span_beyond_line();
    test_apply_span_partially_in_range();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}