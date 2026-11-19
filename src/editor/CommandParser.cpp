#include "editor/CommandParser.h"

// True for space or tab.
static bool isSpaceOrTab(char c)
{
    return c == ' ' || c == '\t';
}

// Splits a line into tokens, handling spaces, tabs, and quoted strings.
DynamicArray<Token> splitLine(const std::string &line)
{
    DynamicArray<Token> tokens;
    int i = 0;
    int n = (int)line.length();

    while (i < n)
    {
        // Skip whitespace.
        while (i < n && isSpaceOrTab(line[i]))
            ++i;
        if (i >= n)
            break;

        Token tok;
        tok.start = i;

        if (line[i] == '"')
        {
            // Quoted token: skip opening quote, read until next quote or end.
            ++i;
            int textStart = i;
            while (i < n && line[i] != '"')
                ++i;
            tok.text = line.substr(textStart, i - textStart);
            if (i < n)
                ++i; // Skip closing quote.
        }
        else
        {
            // Plain token: read until whitespace.
            int textStart = i;
            while (i < n && !isSpaceOrTab(line[i]))
                ++i;
            tok.text = line.substr(textStart, i - textStart);
        }

        tokens.pushBack(tok);
    }

    return tokens;
}

// Returns a lowercase copy of s.
std::string toLowerCase(const std::string &s)
{
    std::string result = s;
    for (int i = 0; i < (int)result.length(); ++i)
    {
        if (result[i] >= 'A' && result[i] <= 'Z')
            result[i] = result[i] - 'A' + 'a';
    }
    return result;
}

// Parses a strictly positive integer with 1 to 9 digits.
bool parsePositiveInt(const std::string &s, int &out)
{
    if (s.empty() || s.length() > 9)
        return false;

    int value = 0;
    for (int i = 0; i < (int)s.length(); ++i)
    {
        char c = s[i];
        if (c < '0' || c > '9')
            return false;
        value = value * 10 + (c - '0');
    }

    if (value < 1)
        return false;

    out = value;
    return true;
}

// Returns the text from the token's start to the end of the line.
// Strips trailing whitespace, and removes matching outer quotes if present.
std::string restOfLine(const std::string &line, const DynamicArray<Token> &tokens, int tokenIndex)
{
    if (tokenIndex < 0 || tokenIndex >= tokens.size())
        return "";

    Token tok;
    if (!tokens.get(tokenIndex, tok))
        return "";

    if (tok.start < 0 || tok.start > (int)line.length())
        return "";

    // Extract the substring from the token's start to the end of the line.
    std::string result = line.substr(tok.start);

    // Strip trailing spaces and tabs.
    int end = (int)result.length() - 1;
    while (end >= 0 && isSpaceOrTab(result[end]))
        --end;
    result = result.substr(0, end + 1);

    // Strip surrounding double quotes if present.
    if (result.length() >= 2 && result[0] == '"' && result[result.length() - 1] == '"')
    {
        result = result.substr(1, result.length() - 2);
    }

    return result;
}