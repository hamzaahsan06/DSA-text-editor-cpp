#include "editor/SearchReplace.h"
#include "ds/DynamicArray.h"
#include "ds/Stack.h"
#include "editor/Editor.h"
#include "editor/Types.h"
#include "editor/UndoRedo.h"

// Returns true if text contains pattern starting exactly at position pos.
// Assumes pos is inside the string and pos + pattern.length() <= text.length().
static bool matchesAt(const std::string &text, const std::string &pattern, int pos)
{
    int m = (int)pattern.length();
    for (int j = 0; j < m; ++j)
    {
        if (text[pos + j] != pattern[j])
        {
            return false;
        }
    }
    return true;
}

// Returns true if a format span overlaps the column range [start, end).
// Overlap means the span is not entirely before start or entirely after end.
static bool spanOverlaps(const FormatSpan &span, int start, int end)
{
    if (span.endCol <= start)
    {
        return false;
    }
    if (span.startCol >= end)
    {
        return false;
    }
    return true;
}

// Rebuilds a line's format spans after replacing oldLen characters at
// replaceCol with newLen characters. Spans entirely before the edit are
// kept; spans entirely after it are shifted; spans overlapping the edit
// are dropped, since their meaning is ambiguous.
static void adjustFormatsAfterReplace(Line &line, int replaceCol,
                                      int oldLen, int newLen)
{
    int delta = newLen - oldLen;
    DynamicArray<FormatSpan> updated;

    int total = line.formats.size();
    for (int i = 0; i < total; ++i)
    {
        FormatSpan span;
        if (!line.formats.get(i, span))
        {
            continue;
        }
        int editEnd = replaceCol + oldLen;
        if (spanOverlaps(span, replaceCol, editEnd))
        {
            continue;
        }
        if (span.startCol >= editEnd)
        {
            span.startCol += delta;
            span.endCol += delta;
        }
        updated.pushBack(span);
    }
    line.formats = updated;
}

// Returns the index of the first match of pattern in text at or after
// startPos, or -1 if there is no match.
int findSubstring(const std::string &text, const std::string &pattern, int startPos)
{
    if (pattern.length() == 0)
    {
        return -1;
    }
    int n = (int)text.length();
    int m = (int)pattern.length();
    if (startPos < 0)
    {
        startPos = 0;
    }
    if (startPos > n)
    {
        return -1;
    }
    if (m > n - startPos)
    {
        return -1;
    }

    for (int i = startPos; i <= n - m; ++i)
    {
        if (matchesAt(text, pattern, i))
        {
            return i;
        }
    }
    return -1;
}

// Collects every match (including overlapping ones) across all lines.
DynamicArray<Position> searchAll(Editor &ed, const std::string &pattern)
{
    DynamicArray<Position> results;
    if (pattern.length() == 0)
    {
        return results;
    }

    int lines = ed.getLineCount();
    for (int li = 0; li < lines; ++li)
    {
        Line *line = ed.getLine(li);
        if (line == nullptr)
        {
            continue;
        }

        int start = 0;
        while (true)
        {
            int idx = findSubstring(line->text, pattern, start);
            if (idx < 0)
            {
                break;
            }
            Position p;
            p.line = li;
            p.col = idx;
            results.pushBack(p);
            start = idx + 1; // allow overlapping matches
        }
    }
    return results;
}

// Returns true if the pattern occurs at least once anywhere in the document.
bool containsText(Editor &ed, const std::string &pattern)
{
    if (pattern.length() == 0)
    {
        return false;
    }
    int lines = ed.getLineCount();
    for (int li = 0; li < lines; ++li)
    {
        Line *line = ed.getLine(li);
        if (line == nullptr)
        {
            continue;
        }
        if (findSubstring(line->text, pattern, 0) >= 0)
        {
            return true;
        }
    }
    return false;
}

// Prints search results: a "Not found" message, or the count and each
// 1-based position.
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

// Builds the text that results from replacing the range
// [replaceCol, replaceCol + oldLen) of source with newText.
static std::string buildReplacedText(const std::string &source, int replaceCol,
                                     int oldLen, const std::string &newText)
{
    std::string result = source.substr(0, replaceCol);
    result += newText;
    result += source.substr(replaceCol + oldLen);
    return result;
}

// Replaces the nth match (1-based) of pattern with newStr. Records the
// change through UndoRedo and adjusts the line's format spans.
bool replaceOccurrence(Editor &ed, const std::string &pattern,
                       const std::string &newStr, int n)
{
    if (pattern.length() == 0)
    {
        return false;
    }
    if (n < 1)
    {
        return false;
    }
    int lines = ed.getLineCount();
    if (lines == 0)
    {
        return false;
    }

    UndoRedo undoRedo;
    int seen = 0;

    for (int li = 0; li < lines; ++li)
    {
        Line *line = ed.getLine(li);
        if (line == nullptr)
        {
            continue;
        }

        int start = 0;
        while (true)
        {
            int idx = findSubstring(line->text, pattern, start);
            if (idx < 0)
            {
                break;
            }
            ++seen;
            if (seen == n)
            {
                std::string updated = buildReplacedText(
                    line->text, idx, (int)pattern.length(), newStr);
                if (!undoRedo.recordReplace(ed, li, idx, pattern, newStr))
                {
                    return false;
                }
                adjustFormatsAfterReplace(*line, idx, (int)pattern.length(),
                                          (int)newStr.length());
                line->text = updated;
                ed.setModified(true);
                return true;
            }
            start = idx + 1; // overlapping matches, matching searchAll
        }
    }
    return false;
}

// Replaces every non-overlapping match left to right. Returns the number
// of replacements. The scan advances past the inserted text so newStr
// containing pattern cannot cause an infinite loop.
int replaceAll(Editor &ed, const std::string &pattern, const std::string &newStr)
{
    if (pattern.length() == 0)
    {
        return 0;
    }
    int lines = ed.getLineCount();
    if (lines == 0)
    {
        return 0;
    }

    UndoRedo undoRedo;
    int total = 0;

    for (int li = 0; li < lines; ++li)
    {
        Line *line = ed.getLine(li);
        if (line == nullptr)
        {
            continue;
        }

        std::string source = line->text;
        std::string rebuilt;
        int start = 0;
        int replacedOnThisLine = 0;
        bool recordFailed = false;

        while (true)
        {
            int idx = findSubstring(source, pattern, start);
            if (idx < 0)
            {
                rebuilt += source.substr(start);
                break;
            }

            rebuilt += source.substr(start, idx - start);

            if (!undoRedo.recordReplace(ed, li, idx, pattern, newStr))
            {
                recordFailed = true;
                break;
            }

            rebuilt += newStr;
            ++total;
            ++replacedOnThisLine;
            start = idx + (int)pattern.length();
        }

        if (recordFailed)
        {
            // Document is unchanged for this line; stop to keep undo
            // history consistent with the text.
            break;
        }

        if (replacedOnThisLine > 0)
        {
            adjustFormatsAfterReplace(*line, 0, (int)source.length(),
                                      (int)rebuilt.length());
            line->text = rebuilt;
        }
    }

    if (total > 0)
    {
        ed.setModified(true);
    }
    return total;
}