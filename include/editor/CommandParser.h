#ifndef EDITOR_COMMAND_PARSER_H
#define EDITOR_COMMAND_PARSER_H

#include <string>
#include "ds/DynamicArray.h"

// A token produced by splitLine.
struct Token
{
    std::string text; // Text of the token. Quotes are removed for quoted tokens.
    int start;        // 0-based index in the original line where the token begins.
};

// Splits a line into tokens. Words are separated by spaces and tabs (any amount).
// A token that begins with a double quote continues until the next double quote
// and may contain spaces; its text does not include the quotes. "" gives an empty
// token. An unterminated quote runs to the end of the line. An empty or all-space
// line gives zero tokens.
DynamicArray<Token> splitLine(const std::string &line);

// Returns a lowercase copy of s (A-Z only; other chars unchanged).
std::string toLowerCase(const std::string &s);

// Parses a strictly positive integer: 1 to 9 digits, no sign, no spaces, value >= 1.
// Returns true and sets out on success; leaves out unchanged on failure.
bool parsePositiveInt(const std::string &s, int &out);

// Returns the text of the original line from tokens[tokenIndex].start to the end,
// with trailing spaces/tabs removed. If the result is wrapped in a pair of double
// quotes (length >= 2, first and last char are quotes), removes them. If the token
// does not exist, returns "".
std::string restOfLine(const std::string &line, const DynamicArray<Token> &tokens, int tokenIndex);

#endif // EDITOR_COMMAND_PARSER_H