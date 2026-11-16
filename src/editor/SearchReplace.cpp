#include "editor/SearchReplace.h"
#include "ds/DynamicArray.h"
#include "ds/Stack.h"
#include "editor/Editor.h"
#include "editor/Types.h"
#include "editor/UndoRedo.h"

// Finds first match of pattern in text at/after startPos; returns -1 if none. O(n*m).
int findSubstring(const std::string &text, const std::string &pattern, int startPos)
{
    if (pattern.empty())
        return -1;
    if (startPos < 0)
        startPos = 0;
    int n = (int)text.length();
    int m = (int)pattern.length();
    if (m > n - startPos)
        return -1;

    for (int i = startPos; i <= n - m; ++i)
    {
        int j = 0;
        while (j < m && text[i + j] == pattern[j])
            ++j;
        if (j == m)
            return i;
    }
    return -1;
}

// Collects every match (overlapping included) across all lines. O(L*n*m).
DynamicArray<Position> searchAll(Editor &ed, const std::string &pattern)
{
    DynamicArray<Position> results;
    if (pattern.empty())
        return results;

    int lines = ed.getLineCount();
    for (int li = 0; li < lines; ++li)
    {
        Line *line = ed.getLine(li);
        if (line == nullptr)
            continue;

        int start = 0;
        while (true)
        {
            int idx = findSubstring(line->text, pattern, start);
            if (idx < 0)
                break;
            Position p;
            p.line = li;
            p.col = idx;
            results.pushBack(p);
            start = idx + 1; // allow overlapping matches
        }
    }
    return results;
}

// Returns true if the pattern occurs at least once. O(L*n*m).
bool containsText(Editor &ed, const std::string &pattern)
{
    if (pattern.empty())
        return false;
    if (ed.getLineCount() == 0)
        return false;

    int lines = ed.getLineCount();
    for (int li = 0; li < lines; ++li)
    {
        Line *line = ed.getLine(li);
        if (line == nullptr)
            continue;
        if (findSubstring(line->text, pattern, 0) >= 0)
            return true;
    }
    return false;
}

// Prints "Not found" or "Found N occurrence(s):" then 1-based positions. O(N).
void displaySearchResults(const DynamicArray<Position> &results, std::ostream &out)
{
    int count = results.size();
    if (count == 0)
    {
        out << "Not found\n";
        return;
    }
    out << "Found " << count << " occurrence(s):\n";
    for (int i = 0; i < count; ++i)
    {
        Position p = results[i];
        out << "Line " << (p.line + 1) << ", Column " << (p.col + 1) << "\n";
    }
}

// Replaces the nth match (1-based); records REPLACE via UndoRedo. O(L*n*m + n).
bool replaceOccurrence(Editor &ed, const std::string &pattern, const std::string &newStr, int n)
{
    if (pattern.empty())
        return false;
    if (n < 1)
        return false;
    if (ed.getLineCount() == 0)
        return false;

    UndoRedo undoRedo;
    int seen = 0;
    int lines = ed.getLineCount();

    for (int li = 0; li < lines; ++li)
    {
        Line *line = ed.getLine(li);
        if (line == nullptr)
            continue;

        int start = 0;
        while (true)
        {
            int idx = findSubstring(line->text, pattern, start);
            if (idx < 0)
                break;
            ++seen;
            if (seen == n)
            {
                // Build replacement string.
                std::string result;
                result.reserve(line->text.length() - pattern.length() + newStr.length());
                for (int k = 0; k < idx; ++k)
                    result += line->text[k];
                result += newStr;
                for (int k = idx + (int)pattern.length(); k < (int)line->text.length(); ++k)
                    result += line->text[k];

                // Record BEFORE mutating (oldText = pattern, newText = newStr).
                if (!undoRedo.recordReplace(ed, li, idx, pattern, newStr))
                    return false;

                line->text = result;

                // TODO: shift format spans
                // (line length changed; format spans after idx may need adjusting.)

                ed.setModified(true);
                return true;
            }
            start = idx + 1; // allow overlapping matches to match searchAll semantics
        }
    }
    return false;
}

// Replaces all matches left-to-right; returns count. Safe when newStr contains pattern.
// O(L*n*m + R*n).
int replaceAll(Editor &ed, const std::string &pattern, const std::string &newStr)
{
    if (pattern.empty())
        return 0;
    if (ed.getLineCount() == 0)
        return 0;

    UndoRedo undoRedo;
    int total = 0;
    int lines = ed.getLineCount();

    for (int li = 0; li < lines; ++li)
    {
        Line *line = ed.getLine(li);
        if (line == nullptr)
            continue;

        std::string src = line->text;
        std::string out;
        out.reserve(src.length());

        int start = 0;
        int replacedOnThisLine = 0;
        while (true)
        {
            int idx = findSubstring(src, pattern, start);
            if (idx < 0)
            {
                for (int k = start; k < (int)src.length(); ++k)
                    out += src[k];
                break;
            }

            for (int k = start; k < idx; ++k)
                out += src[k];

            // Record each replacement through UndoRedo.
            if (!undoRedo.recordReplace(ed, li, idx, pattern, newStr))
                break;

            for (int k = 0; k < (int)newStr.length(); ++k)
                out += newStr[k];

            ++total;
            ++replacedOnThisLine;

            // Continue AFTER the inserted text to avoid infinite loops.
            start = idx + (int)pattern.length();
        }

        if (replacedOnThisLine > 0)
        {
            line->text = out;
            // TODO: shift format spans
            // (line length changed; format spans may need adjusting.)
        }
    }

    if (total > 0)
    {
        ed.setModified(true);
    }
    return total;
}