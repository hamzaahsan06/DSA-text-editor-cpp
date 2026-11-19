#include "editor/SearchReplace.h"
#include "ds/DynamicArray.h"
#include "ds/Stack.h"
#include "editor/Editor.h"
#include "editor/Types.h"
#include "editor/UndoRedo.h"

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
            start = idx + 1;
        }
    }
    return results;
}

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

static std::string buildReplacedText(const std::string &source, int replaceCol,
                                     int oldLen, const std::string &newText)
{
    std::string result = source.substr(0, replaceCol);
    result += newText;
    result += source.substr(replaceCol + oldLen);
    return result;
}

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
            start = idx + 1;
        }
    }
    return false;
}

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
    int patLen = (int)pattern.length();
    int newLen = (int)newStr.length();
    int delta = newLen - patLen;

    for (int li = 0; li < lines; ++li)
    {
        Line *line = ed.getLine(li);
        if (line == nullptr)
        {
            continue;
        }

        std::string source = line->text;
        std::string rebuilt;
        DynamicArray<int> liveCols;
        int start = 0;
        int shift = 0;
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

            int liveCol = idx + shift;
            if (!undoRedo.recordReplace(ed, li, liveCol, pattern, newStr))
            {
                recordFailed = true;
                break;
            }

            liveCols.pushBack(liveCol);
            rebuilt += newStr;
            ++total;
            ++replacedOnThisLine;
            shift += delta;
            start = idx + patLen;
        }

        if (recordFailed)
        {
            // Roll back partial undo entries recorded for this unmodified line.
            for (int r = 0; r < replacedOnThisLine; ++r)
            {
                Command discarded;
                ed.undoStack.pop(discarded);
            }
            break;
        }

        if (replacedOnThisLine > 0)
        {
            for (int r = 0; r < liveCols.size(); ++r)
            {
                int col = 0;
                liveCols.get(r, col);
                adjustFormatsAfterReplace(*line, col, patLen, newLen);
            }
            line->text = rebuilt;
        }
    }

    if (total > 0)
    {
        ed.setModified(true);
    }
    return total;
}