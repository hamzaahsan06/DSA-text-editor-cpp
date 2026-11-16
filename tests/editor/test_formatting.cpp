// g++ -std=c++17 -Wall -Wextra -Iinclude tests/editor/test_Formatting.cpp src/editor/Formatting.cpp -o test_formatting.exe

//.\test_formatting.exe

#include <iostream>
#include <string>
#include "editor/Formatting.h"

static int passed = 0, failed = 0;

#define CHECK(cond)                                                   \
    do                                                                \
    {                                                                 \
        if (cond)                                                     \
            ++passed;                                                 \
        else                                                          \
        {                                                             \
            ++failed;                                                 \
            std::cout << "FAIL line " << __LINE__ << ": " #cond "\n"; \
        }                                                             \
    } while (0)

// Expected: A newly created Formatting object has no formatting spans.
void test_initial_state_empty()
{
    Formatting fmt;
    CHECK(fmt.count() == 0);
}

// Expected: Adding bold formatting increases the span count.
void test_add_bold_format()
{
    Formatting fmt;
    CHECK(fmt.addFormat(0, 4, true, false) == true);
    CHECK(fmt.count() == 1);
}

// Expected: Adding italic formatting increases the span count.
void test_add_italic_format()
{
    Formatting fmt;
    CHECK(fmt.addFormat(0, 4, false, true) == true);
    CHECK(fmt.count() == 1);
}

// Expected: Both bold and italic can be applied to the same range.
void test_add_bold_italic_format()
{
    Formatting fmt;
    CHECK(fmt.addFormat(0, 4, true, true) == true);
    CHECK(fmt.count() == 1);
}

// Expected: Multiple formatting spans can be stored.
void test_add_multiple_formats()
{
    Formatting fmt;
    CHECK(fmt.addFormat(0, 4, true, false) == true);
    CHECK(fmt.addFormat(6, 10, false, true) == true);
    CHECK(fmt.count() == 2);
}

// Expected: Overlapping formatting ranges can be stored.
void test_overlapping_formats()
{
    Formatting fmt;
    CHECK(fmt.addFormat(0, 5, true, false) == true);
    CHECK(fmt.addFormat(3, 8, false, true) == true);
    CHECK(fmt.count() == 2);
}

// Expected: Formatting with negative start position is rejected.
void test_invalid_negative_start()
{
    Formatting fmt;
    CHECK(fmt.addFormat(-1, 4, true, false) == false);
    CHECK(fmt.count() == 0);
}

// Expected: An end position smaller than the start position is rejected.
void test_invalid_range()
{
    Formatting fmt;
    CHECK(fmt.addFormat(5, 2, true, false) == false);
    CHECK(fmt.count() == 0);
}

// Expected: A formatting span with neither bold nor italic is rejected.
void test_invalid_no_format()
{
    Formatting fmt;
    CHECK(fmt.addFormat(0, 4, false, false) == false);
    CHECK(fmt.count() == 0);
}

// Expected: Removing an existing formatting span decreases the count.
void test_remove_format()
{
    Formatting fmt;
    fmt.addFormat(0, 4, true, false);
    fmt.addFormat(6, 10, false, true);

    CHECK(fmt.removeFormat(0) == true);
    CHECK(fmt.count() == 1);
}

// Expected: Removing an invalid index fails safely.
void test_remove_invalid_index()
{
    Formatting fmt;
    fmt.addFormat(0, 4, true, false);

    CHECK(fmt.removeFormat(-1) == false);
    CHECK(fmt.removeFormat(5) == false);
    CHECK(fmt.count() == 1);
}

// Expected: Clearing formatting removes all stored spans.
void test_clear_formats()
{
    Formatting fmt;
    fmt.addFormat(0, 4, true, false);
    fmt.addFormat(6, 10, false, true);

    CHECK(fmt.count() == 2);

    fmt.clearFormats();

    CHECK(fmt.count() == 0);
}

// Expected: Applying formatting to an empty string returns an empty string.
void test_empty_string()
{
    Formatting fmt;
    fmt.addFormat(0, 4, true, false);

    CHECK(fmt.applyFormatting("") == "");
}

// Expected: Applying formatting without any spans preserves the original text.
void test_no_format_preserves_text()
{
    Formatting fmt;
    std::string text = "Hello World";

    CHECK(fmt.applyFormatting(text) == text);
}

// Expected: Applying bold formatting adds the ANSI bold escape sequence.
void test_apply_bold_format()
{
    Formatting fmt;
    fmt.addFormat(0, 4, true, false);

    std::string result = fmt.applyFormatting("Hello");

    CHECK(result.find("\033[1m") != std::string::npos);
    CHECK(result.find("H") != std::string::npos);
    CHECK(result.find("\033[0m") != std::string::npos);
}

// Expected: Applying italic formatting adds the ANSI italic escape sequence.
void test_apply_italic_format()
{
    Formatting fmt;
    fmt.addFormat(0, 4, false, true);

    std::string result = fmt.applyFormatting("Hello");

    CHECK(result.find("\033[3m") != std::string::npos);
    CHECK(result.find("H") != std::string::npos);
    CHECK(result.find("\033[0m") != std::string::npos);
}

// Expected: Applying both styles adds both ANSI escape sequences.
void test_apply_bold_italic_format()
{
    Formatting fmt;
    fmt.addFormat(0, 4, true, true);

    std::string result = fmt.applyFormatting("Hello");

    CHECK(result.find("\033[1m") != std::string::npos);
    CHECK(result.find("\033[3m") != std::string::npos);
}

// Expected: Formatting beyond the string length does not crash.
void test_format_beyond_text_length()
{
    Formatting fmt;
    CHECK(fmt.addFormat(0, 100, true, false) == true);

    std::string result = fmt.applyFormatting("Hello");

    CHECK(!result.empty());
}

int main()
{
    test_initial_state_empty();
    test_add_bold_format();
    test_add_italic_format();
    test_add_bold_italic_format();
    test_add_multiple_formats();
    test_overlapping_formats();
    test_invalid_negative_start();
    test_invalid_range();
    test_invalid_no_format();
    test_remove_format();
    test_remove_invalid_index();
    test_clear_formats();
    test_empty_string();
    test_no_format_preserves_text();
    test_apply_bold_format();
    test_apply_italic_format();
    test_apply_bold_italic_format();
    test_format_beyond_text_length();

    std::cout << passed << " passed, " << failed << " failed\n";

    return failed == 0 ? 0 : 1;
}