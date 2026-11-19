#include "editor/TextOps.h"

#include <iostream>
#include "editor/Validation.h"
#include "editor/UndoRedo.h"
#include "editor/Bookmarks.h"

// True for space, tab, newline, carriage return, vertical tab and form feed.
static bool isWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

static bool requireLine(const Editor &ed, int line)
{
    if (isDocumentEmpty(ed))
    {
        std::cout << "Error: the document is empty.\n";
        return false;
    }
    if (!isValidLine(ed, line))
    {
        std::cout << "Error: line " << (line + 1) << " does not exist (document has "
                  << ed.document.size() << " lines).\n";
        return false;
    }
    return true;
}

static bool requireColumn(const Editor &ed, int line, int col)
{
    if (!isValidColumn(ed, line, col))
    {
        std::cout << "Error: column " << (col + 1) << " is outside line " << (line + 1) << ".\n";
        return false;
    }
    return true;
}

static bool openEmptyDocument(Editor &ed, int line, int col)
{
    if (!isDocumentEmpty(ed))
        return true;
    if (line != 0 || col != 0)
    {
        std::cout << "Error: the document is empty; insert at line 1, column 1.\n";
        return false;
    }
    Line first;
    ed.document.insertAtTail(first);
    return true;
}

static void moveCursor(Editor &ed, int line, int col)
{
    ed.cursorLine = line;
    ed.cursorCol = col;
}

// Shift spans to the right on insert.
static void shiftSpansOnInsert(Line &line, int col, int length)
{
    for (int i = 0; i < line.formats.size(); ++i)
    {
        FormatSpan span;
        if (!line.formats.get(i, span))
            continue;
        if (span.startCol >= col)
            span.startCol += length;
        // endCol is exclusive: text typed exactly at the end of a span is
        // outside it, so the span only grows when the insert is strictly inside.
        if (span.endCol > col)
            span.endCol += length;
        line.formats.set(i, span);
    }
}

// Shift/shrink spans on delete.
static void shiftSpansOnDelete(Line &line, int startCol, int endCol)
{
    int length = endCol - startCol;
    for (int i = 0; i < line.formats.size(); ++i)
    {
        FormatSpan span;
        if (!line.formats.get(i, span))
            continue;

        if (span.startCol >= endCol)
        {
            span.startCol -= length;
            span.endCol -= length;
            line.formats.set(i, span);
        }
        else if (span.endCol > startCol)
        {
            if (span.startCol >= startCol)
                span.startCol = startCol;
            if (span.endCol <= endCol)
                span.endCol = startCol;
            else
                span.endCol -= length;

            if (span.startCol >= span.endCol)
            {
                FormatSpan dummy;
                line.formats.removeAt(i, dummy);
                --i;
            }
            else
            {
                line.formats.set(i, span);
            }
        }
    }
}

// Splits the spans of `first` at column `col` when its text is cut in two.
// Spans ending at or before col stay on `first`; spans starting at or after col
// move to `second` with their columns reduced by col; a span that crosses col is
// cut into two pieces, one on each line.
static void splitSpansAtColumn(Line &first, Line &second, int col)
{
    DynamicArray<FormatSpan> keep;
    DynamicArray<FormatSpan> moved;

    for (int i = 0; i < first.formats.size(); ++i)
    {
        FormatSpan span;
        if (!first.formats.get(i, span))
            continue;

        if (span.endCol <= col)
        {
            keep.pushBack(span);
        }
        else if (span.startCol >= col)
        {
            span.startCol -= col;
            span.endCol -= col;
            moved.pushBack(span);
        }
        else
        {
            FormatSpan head = span;
            head.endCol = col;
            keep.pushBack(head);

            FormatSpan tail = span;
            tail.startCol = 0;
            tail.endCol = span.endCol - col;
            moved.pushBack(tail);
        }
    }

    first.formats = keep;
    second.formats = moved;
}

bool insertText(Editor &ed, int line, int col, const std::string &text)
{
    if (isEmptyString(text))
    {
        std::cout << "Error: there is no text to insert.\n";
        return false;
    }
    if (text.find('\n') != std::string::npos)
    {
        std::cout << "Error: text cannot contain a newline; use insert new line.\n";
        return false;
    }
    const bool createdFirstLine = isDocumentEmpty(ed);
    if (!openEmptyDocument(ed, line, col))
        return false;
    if (!requireLine(ed, line) || !requireColumn(ed, line, col))
        return false;

    Line *target = ed.getLine(line);
    UndoRedo history;
    if (!history.recordInsert(ed, line, col, text))
    {
        if (createdFirstLine)
        {
            Line discarded;
            ed.document.removeAtPosition(0, discarded);
        }
        std::cout << "Error: could not record the edit in undo history.\n";
        return false;
    }

    target->text.insert(col, text);
    shiftSpansOnInsert(*target, col, static_cast<int>(text.size()));

    ed.setModified(true);
    moveCursor(ed, line, col + static_cast<int>(text.size()));
    return true;
}

bool insertNewLine(Editor &ed, int line, int col)
{
    const bool createdFirstLine = isDocumentEmpty(ed);
    if (!openEmptyDocument(ed, line, col))
        return false;
    if (!requireLine(ed, line) || !requireColumn(ed, line, col))
        return false;

    Line *target = ed.getLine(line);
    std::string originalText = target->text;

    // Record first so a full undo stack cannot leave an untracked edit.
    UndoRedo history;
    Command op;
    op.type = INSERT_LINE;
    op.line = line;
    op.col = col;
    op.newText = originalText;
    op.lineSplit = true;
    if (!history.record(ed, op))
    {
        if (createdFirstLine)
        {
            Line discarded;
            ed.document.removeAtPosition(0, discarded);
        }
        std::cout << "Error: could not record the edit in undo history.\n";
        return false;
    }

    Line below;
    below.text = target->text.substr(col);
    target->text.erase(col);
    splitSpansAtColumn(*target, below, col);
    ed.document.insertAtPosition(line + 1, below);
    adjustBookmarksOnLineInsert(ed, line + 1);

    ed.setModified(true);
    moveCursor(ed, line + 1, 0);
    return true;
}

bool deleteText(Editor &ed, int line, int startCol, int endCol)
{
    if (!requireLine(ed, line))
        return false;
    if (!isValidColumnRange(ed, line, startCol, endCol))
    {
        std::cout << "Error: columns " << (startCol + 1) << " to " << endCol
                  << " are not inside line " << (line + 1) << ".\n";
        return false;
    }
    if (startCol == endCol)
    {
        std::cout << "Error: nothing to delete (empty range).\n";
        return false;
    }

    Line *target = ed.getLine(line);
    std::string removed = target->text.substr(startCol, endCol - startCol);

    UndoRedo history;
    if (!history.recordErase(ed, line, startCol, removed))
    {
        std::cout << "Error: could not record the edit in undo history.\n";
        return false;
    }

    target->text.erase(startCol, endCol - startCol);
    shiftSpansOnDelete(*target, startCol, endCol);

    ed.setModified(true);
    moveCursor(ed, line, startCol);
    return true;
}

bool deleteChar(Editor &ed, int line, int col)
{
    if (!requireLine(ed, line))
        return false;

    int length = static_cast<int>(ed.getLine(line)->text.size());
    if (col < 0 || col >= length)
    {
        std::cout << "Error: there is no character at column " << (col + 1)
                  << " of line " << (line + 1) << ".\n";
        return false;
    }
    return deleteText(ed, line, col, col + 1);
}

bool deleteWord(Editor &ed, int line, int col)
{
    if (!requireLine(ed, line))
        return false;

    const std::string &text = ed.getLine(line)->text;
    int length = static_cast<int>(text.size());
    if (col < 0 || col >= length || isWhitespace(text[col]))
    {
        std::cout << "Error: there is no word at line " << (line + 1)
                  << ", column " << (col + 1) << ".\n";
        return false;
    }

    int start = col;
    while (start > 0 && !isWhitespace(text[start - 1]))
        --start;
    int end = col;
    while (end < length && !isWhitespace(text[end]))
        ++end;

    return deleteText(ed, line, start, end);
}

bool deleteLines(Editor &ed, int startLine, int endLine)
{
    if (isDocumentEmpty(ed))
    {
        std::cout << "Error: the document is empty.\n";
        return false;
    }
    if (!isValidRange(ed, startLine, endLine))
    {
        std::cout << "Error: lines " << (startLine + 1) << " to " << (endLine + 1)
                  << " are not a valid range (document has " << ed.document.size()
                  << " lines).\n";
        return false;
    }

    // Gather the text BEFORE removing the lines.
    std::string removedText;
    for (int i = startLine; i <= endLine; ++i)
    {
        Line *l = ed.getLine(i);
        if (l != nullptr)
        {
            if (i > startLine)
                removedText += '\n';
            removedText += l->text;
        }
    }

    UndoRedo history;
    // recordDeleteLine checks that line < getLineCount()
    if (!history.recordDeleteLine(ed, startLine, removedText))
        return false;

    int count = endLine - startLine + 1;
    for (int i = 0; i < count; ++i)
    {
        Line removed;
        ed.document.removeAtPosition(startLine, removed);
        adjustBookmarksOnLineDelete(ed, startLine);
    }

    ed.setModified(true);
    int remaining = ed.document.size();
    if (remaining == 0)
        moveCursor(ed, 0, 0);
    else
        moveCursor(ed, startLine < remaining ? startLine : remaining - 1, 0);
    return true;
}
