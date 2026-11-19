#include "editor/Formatting.h"
#include "ds/DynamicArray.h"
#include <iostream>

// Helper: checks if a given column is covered by a bold or italic span.
// Ignore spans that reach past the line length, per the safe-guard rule.
static void getStyleAt(const Line &line, int col, bool &isBold, bool &isItalic)
{
    isBold = false;
    isItalic = false;
    int textLen = (int)line.text.length();
    if (col < 0 || col >= textLen)
        return;

    for (int i = 0; i < line.formats.size(); ++i)
    {
        FormatSpan span;
        if (!line.formats.get(i, span))
            continue;

        int s = span.startCol;
        int e = span.endCol;
        // Ignore invalid parts that reach past the text length
        if (e > textLen)
            e = textLen;

        if (col >= s && col < e)
        {
            if (span.type == BOLD || span.type == BOLD_ITALIC)
                isBold = true;
            if (span.type == ITALIC || span.type == BOLD_ITALIC)
                isItalic = true;
        }
    }
}

// Adds a format span if the range is valid and at least one style is true.
bool addFormat(Line &line, int startCol, int endCol, bool isBold, bool isItalic)
{
    if (startCol < 0 || startCol >= endCol)
        return false;
    if (endCol > (int)line.text.length())
        return false;
    if (!isBold && !isItalic)
        return false;

    FormatSpan span;
    span.startCol = startCol;
    span.endCol = endCol;

    if (isBold && isItalic)
        span.type = BOLD_ITALIC;
    else if (isBold)
        span.type = BOLD;
    else
        span.type = ITALIC;

    return line.formats.pushBack(span);
}

// Removes the format span at the given index.
bool removeFormat(Line &line, int spanIndex)
{
    if (spanIndex < 0 || spanIndex >= line.formats.size())
        return false;
    FormatSpan dummy;
    return line.formats.removeAt(spanIndex, dummy);
}

// Clears all format spans.
void clearFormats(Line &line)
{
    line.formats.clear();
}

// Renders the line with ANSI codes or text-based fallback markers.
std::string renderLine(const Line &line, bool useAnsi)
{
    std::string result = "";
    int textLen = (int)line.text.length();
    if (textLen == 0)
        return result;

    bool prevBold = false;
    bool prevItalic = false;
    bool first = true;

    for (int i = 0; i < textLen; ++i)
    {
        bool curBold, curItalic;
        getStyleAt(line, i, curBold, curItalic);

        if (useAnsi)
        {
            if (first)
            {
                if (curBold)
                    result += "\033[1m";
                if (curItalic)
                    result += "\033[3m";
            }
            else
            {
                if (curBold != prevBold || curItalic != prevItalic)
                {
                    // Reset previous run if we were in a styled state
                    if (prevBold || prevItalic)
                        result += "\033[0m";
                    // Start new run
                    if (curBold)
                        result += "\033[1m";
                    if (curItalic)
                        result += "\033[3m";
                }
            }
            result += line.text[i];
        }
        else
        {
            // Fallback: insert markers at transitions
            if (first)
            {
                if (curBold && curItalic)
                    result += "***";
                else if (curBold)
                    result += "**";
                else if (curItalic)
                    result += "*";
            }
            else
            {
                if (curBold != prevBold || curItalic != prevItalic)
                {
                    // Close previous markers if any
                    if (prevBold && prevItalic)
                        result += "***";
                    else if (prevBold)
                        result += "**";
                    else if (prevItalic)
                        result += "*";
                    // Open new markers
                    if (curBold && curItalic)
                        result += "***";
                    else if (curBold)
                        result += "**";
                    else if (curItalic)
                        result += "*";
                }
            }
            result += line.text[i];
        }

        prevBold = curBold;
        prevItalic = curItalic;
        first = false;
    }

    // Close out the final run
    if (useAnsi)
    {
        if (prevBold || prevItalic)
            result += "\033[0m";
    }
    else
    {
        if (prevBold && prevItalic)
            result += "***";
        else if (prevBold)
            result += "**";
        else if (prevItalic)
            result += "*";
    }

    return result;
}