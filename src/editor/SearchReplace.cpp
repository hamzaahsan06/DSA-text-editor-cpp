#include "editor/SearchReplace.h"
#include "ds/DynamicArray.h"
#include "ds/Stack.h"
#include "editor/Editor.h"
#include "editor/Types.h"
#include "editor/UndoRedo.h"
#include <iostream>

// Returns true if text contains pattern starting exactly at position pos.
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
static bool spanOverlaps(const FormatSpan &span, int start, int end)
{
    if (span.endCol <= start)
        return false;
    if (span.startCol >= end)
        return false;
    return true;
}

// Rebuilds a line's format spans after replacing oldLen characters at replaceCol with newLen characters.
static void adjustFormatsAfterReplace(Line &line, int replaceCol, int oldLen, int newLen)
{
    int delta = newLen - oldLen;
    DynamicArray<FormatSpan> updated;

    int total = line.formats.size();
    for (int i = 0; i < total; ++i)
    {
        FormatSpan span;
        if (!line.formats.get(i, span))
            continue;

        int editEnd = replaceCol + oldLen;
        if (spanOverlaps(span, replaceCol, editEnd))
            continue;

        if (span.startCol >= editEnd)
        {
            span.startCol += delta;
            span.endCol += delta;
        }
        updated.pushBack(span);
    }
    line.formats = updated;
}

// Returns the index of the first match of pattern in text at or after startPos.
int findSubstring(const std::string &text, const std::string &pattern, int startPos)
{
    if (pattern.length() == 0)
        return -1;
    int n = (int)text.length();
    int m = (int)pattern.length();
    if (startPos < 0)
        startPos = 0;
    if (startPos > n)
        return -1;
    if (m > n - startPos)
        return -1;

    for (int i = startPos; i <= n - m; ++i)
    {
        if (matchesAt(text, pattern, i))
            return i;
    }
    return -1;
}

// Collects every match (including overlapping ones) across all lines.
DynamicArray<Position> searchAll(Editor &ed, const std::string &pattern)
{
    DynamicArray<Position> results;
    if (pattern.length() == 0)
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

// Returns true if the pattern occurs at least once anywhere in the document.
bool containsText(Editor &ed, const std::string &pattern)
{
    if (pattern.length() == 0)
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

// Prints search results.
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

// Builds the text that results from replacing the range [replaceCol, replaceCol + oldLen).
static std::string buildReplacedText(const std::string &source, int replaceCol, int oldLen, const std::string &newText)
{
    std::string result = source.substr(0, replaceCol);
    result += newText;
    result += source.substr(replaceCol + oldLen);
    return result;
}

// Replaces the nth match (1-based) of pattern with newStr.
bool replaceOccurrence(Editor &ed, const std::string &pattern, const std::string &newStr, int n)
{
    if (pattern.length() == 0 || n < 1)
        return false;
    int lines = ed.getLineCount();
    if (lines == 0)
        return false;

    int seen = 0;
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
                Position cb;
                cb.line = ed.cursorLine;
                cb.col = ed.cursorCol;
                Line before = *line;

                std::string updated = buildReplacedText(line->text, idx, (int)pattern.length(), newStr);
                adjustFormatsAfterReplace(*line, idx, (int)pattern.length(), (int)newStr.length());
                line->text = updated;

                ed.cursorLine = li;
                ed.cursorCol = idx + (int)newStr.length();
                ed.setModified(true);

                UndoRedo undoRedo;
                if (!undoRedo.recordLineEdit(ed, REPLACE, li, before, cb))
                {
                    std::cout << "Warning: this change cannot be undone.\n";
                }
                return true;
            }
            start = idx + 1; // overlapping matches
        }
    }
    return false;
}

// Replaces every non-overlapping match left to right. Returns the number of replacements.
int replaceAll(Editor &ed, const std::string &pattern, const std::string &newStr)
{
    if (pattern.length() == 0)
        return 0;
    int lines = ed.getLineCount();
    if (lines == 0)
        return 0;

    int firstLine = -1;
    int lastLine = -1;
    for (int li = 0; li < lines; ++li)
    {
        Line *line = ed.getLine(li);
        if (line != nullptr && findSubstring(line->text, pattern, 0) >= 0)
        {
            if (firstLine == -1)
                firstLine = li;
            lastLine = li;
        }
    }

    if (firstLine == -1)
        return 0; // No matches

    Position cb;
    cb.line = ed.cursorLine;
    cb.col = ed.cursorCol;
    int blockSize = lastLine - firstLine + 1;
    DynamicArray<Line> beforeBlock;

    // Walk nodes to copy the block BEFORE changing anything.
    DoublyLinkedList<Line>::Node *cur = ed.document.getHead();
    for (int i = 0; i < firstLine && cur != nullptr; ++i)
        cur = cur->next;
    for (int i = 0; i < blockSize && cur != nullptr; ++i)
    {
        beforeBlock.pushBack(cur->data);
        cur = cur->next;
    }

    int total = 0;
    int cursorAfterLine = firstLine;
    int cursorAfterCol = 0;

    // Now process the block
    for (int li = firstLine; li <= lastLine; ++li)
    {
        Line *line = ed.getLine(li);
        if (line == nullptr)
            continue;

        std::string source = line->text;
        std::string rebuilt;
        int start = 0;
        int replacedOnThisLine = 0;

        while (true)
        {
            int idx = findSubstring(source, pattern, start);
            if (idx < 0)
            {
                rebuilt += source.substr(start);
                break;
            }

            rebuilt += source.substr(start, idx - start);
            adjustFormatsAfterReplace(*line, idx, (int)pattern.length(), (int)newStr.length());
            rebuilt += newStr;
            ++total;
            ++replacedOnThisLine;
            start = idx + (int)pattern.length();
            cursorAfterLine = li;
            cursorAfterCol = start;
        }

        if (replacedOnThisLine > 0)
        {
            line->text = rebuilt;
        }
    }

    ed.cursorLine = cursorAfterLine;
    ed.cursorCol = cursorAfterCol;
    ed.setModified(true);

    UndoRedo undoRedo;
    if (!undoRedo.recordBlockEdit(ed, REPLACE_ALL, firstLine, beforeBlock, blockSize, cb))
    {
        std::cout << "Warning: this change cannot be undone.\n";
    }

    return total;
}