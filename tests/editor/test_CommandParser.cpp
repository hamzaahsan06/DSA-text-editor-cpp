// g++ -std=c++11 -Wall -Wextra -Iinclude -Iinclude/editor tests/editor/test_CommandParser.cpp src/editor/CommandParser.cpp -o build/test_CommandParser ; if ($?) { ./build/test_CommandParser }

#include <iostream>
#include <string>
#include "editor/CommandParser.h"
#include "../ds/check.h"

// ---------- splitLine ----------

// Splits words separated by single spaces.
void test_split_simple()
{
    DynamicArray<Token> t = splitLine("hello world foo");
    CHECK(t.size() == 3);
    Token tok;
    t.get(0, tok);
    CHECK(tok.text == "hello" && tok.start == 0);
    t.get(1, tok);
    CHECK(tok.text == "world" && tok.start == 6);
    t.get(2, tok);
    CHECK(tok.text == "foo" && tok.start == 12);
}

// Ignores tabs and multiple spaces.
void test_split_tabs_and_spaces()
{
    DynamicArray<Token> t = splitLine("  one\t\ttwo   three  ");
    CHECK(t.size() == 3);
    Token tok;
    t.get(0, tok);
    CHECK(tok.text == "one");
    t.get(1, tok);
    CHECK(tok.text == "two");
    t.get(2, tok);
    CHECK(tok.text == "three");
}

// Empty or all-space line gives zero tokens.
void test_split_empty()
{
    CHECK(splitLine("").size() == 0);
    CHECK(splitLine("   ").size() == 0);
    CHECK(splitLine("\t\t").size() == 0);
}

// Quoted token may contain spaces.
void test_split_quoted()
{
    DynamicArray<Token> t = splitLine("say \"hello world\" now");
    CHECK(t.size() == 3);
    Token tok;
    t.get(1, tok);
    CHECK(tok.text == "hello world");
    CHECK(tok.start == 4); // position of the opening quote
}

// Empty quotes give an empty token.
void test_split_empty_quotes()
{
    DynamicArray<Token> t = splitLine("a \"\" b");
    CHECK(t.size() == 3);
    Token tok;
    t.get(1, tok);
    CHECK(tok.text == "");
}

// Unterminated quote runs to the end of the line.
void test_split_unterminated_quote()
{
    DynamicArray<Token> t = splitLine("say \"no close");
    CHECK(t.size() == 2);
    Token tok;
    t.get(1, tok);
    CHECK(tok.text == "no close");
}

// ---------- toLowerCase ----------

// Lowercases A-Z only.
void test_to_lowercase()
{
    CHECK(toLowerCase("Hello WORLD 123 _-!") == "hello world 123 _-!");
    CHECK(toLowerCase("") == "");
}

// ---------- parsePositiveInt ----------

// Accepts 1..999999999.
void test_parse_valid_numbers()
{
    int v = 0;
    CHECK(parsePositiveInt("1", v) == true && v == 1);
    CHECK(parsePositiveInt("42", v) == true && v == 42);
    CHECK(parsePositiveInt("999999999", v) == true && v == 999999999);
}

// Rejects every bad input listed in the spec.
void test_parse_bad_numbers()
{
    int v = -1;
    CHECK(parsePositiveInt("0", v) == false && v == -1);
    CHECK(parsePositiveInt("-5", v) == false && v == -1);
    CHECK(parsePositiveInt("+5", v) == false && v == -1);
    CHECK(parsePositiveInt("12a", v) == false && v == -1);
    CHECK(parsePositiveInt("", v) == false && v == -1);
    CHECK(parsePositiveInt(" 5", v) == false && v == -1);
    CHECK(parsePositiveInt("1000000000", v) == false && v == -1);
    CHECK(parsePositiveInt("5 ", v) == false && v == -1);
}

// ---------- restOfLine ----------

// Rest of line from token 3 (plain token) trims trailing spaces.
void test_rest_plain()
{
    DynamicArray<Token> t = splitLine("ins 3 5 hello   world   ");
    CHECK(restOfLine("ins 3 5 hello   world   ", t, 3) == "hello   world");
}

// Multiple spaces inside the text are preserved.
void test_rest_preserves_inner_spaces()
{
    DynamicArray<Token> t = splitLine("ins 3 5 hello   world");
    CHECK(restOfLine("ins 3 5 hello   world", t, 3) == "hello   world");
}

// Quoted text keeps leading/trailing spaces inside the quotes.
void test_rest_quoted()
{
    DynamicArray<Token> t = splitLine("ins 3 5 \"  spaced  \"");
    CHECK(restOfLine("ins 3 5 \"  spaced  \"", t, 3) == "  spaced  ");
}

// Missing token returns "".
void test_rest_missing_token()
{
    DynamicArray<Token> t = splitLine("only one");
    CHECK(restOfLine("only one", t, 5) == "");
    CHECK(restOfLine("only one", t, -1) == "");
}

// Zero tokens returns "".
void test_rest_no_tokens()
{
    DynamicArray<Token> t = splitLine("   ");
    CHECK(restOfLine("   ", t, 0) == "");
}

// Single-character plain token.
void test_rest_single_token()
{
    DynamicArray<Token> t = splitLine("hi");
    CHECK(restOfLine("hi", t, 0) == "hi");
}

// Full example from the spec.
void test_rest_ins_example()
{
    const std::string line = "ins 3 5 hello   world";
    DynamicArray<Token> t = splitLine(line);
    CHECK(t.size() == 5);
    CHECK(restOfLine(line, t, 3) == "hello   world");
}

int main()
{
    test_split_simple();
    test_split_tabs_and_spaces();
    test_split_empty();
    test_split_quoted();
    test_split_empty_quotes();
    test_split_unterminated_quote();

    test_to_lowercase();

    test_parse_valid_numbers();
    test_parse_bad_numbers();

    test_rest_plain();
    test_rest_preserves_inner_spaces();
    test_rest_quoted();
    test_rest_missing_token();
    test_rest_no_tokens();
    test_rest_single_token();
    test_rest_ins_example();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}