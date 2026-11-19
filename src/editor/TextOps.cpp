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
    if (!openEmptyDocument(ed, line, col))
        return false;
    if (!requireLine(ed, line) || !requireColumn(ed, line, col))
        return false;

    Position cb;
    cb.line = ed.cursorLine;
    cb.col = ed.cursorCol;
    Line before = *ed.getLine(line);

    Line *target = ed.getLine(line);
    target->text.insert(col, text);
    shiftSpansOnInsert(*target, col, static_cast<int>(text.size()));

    ed.setModified(true);
    moveCursor(ed, line, col + static_cast<int>(text.size()));

    UndoRedo history;
    if (!history.recordLineEdit(ed, INSERT_TEXT, line, before, cb))
        std::cout << "Warning: this change cannot be undone.\n";

    return true;
}

bool insertNewLine(Editor &ed, int line, int col)
{
    if (!openEmptyDocument(ed, line, col))
        return false;
    if (!requireLine(ed, line) || !requireColumn(ed, line, col))
        return false;

    Position cb;
    cb.line = ed.cursorLine;
    cb.col = ed.cursorCol;
    Line before = *ed.getLine(line);

    Line *target = ed.getLine(line);
    Line below;
    below.text = target->text.substr(col);
    target->text.erase(col);
    splitSpansAtColumn(*target, below, col);
    ed.document.insertAtPosition(line + 1, below);
    adjustBookmarksOnSplit(ed, line, col);

    ed.setModified(true);
    moveCursor(ed, line + 1, 0);

    UndoRedo history;
    if (!history.recordSplit(ed, line, col, before, cb))
        std::cout << "Warning: this change cannot be undone.\n";

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

    Position cb;
    cb.line = ed.cursorLine;
    cb.col = ed.cursorCol;
    Line before = *ed.getLine(line);

    Line *target = ed.getLine(line);
    target->text.erase(startCol, endCol - startCol);
    shiftSpansOnDelete(*target, startCol, endCol);

    ed.setModified(true);
    moveCursor(ed, line, startCol);

    UndoRedo history;
    if (!history.recordLineEdit(ed, DELETE_TEXT, line, before, cb))
        std::cout << "Warning: this change cannot be undone.\n";

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

    Position cb;
    cb.line = ed.cursorLine;
    cb.col = ed.cursorCol;

    // Copy the lines block
    DynamicArray<Line> deletedLines;
    DoublyLinkedList<Line>::Node *cur = ed.document.getHead();
    for (int i = 0; i < startLine && cur != nullptr; ++i)
    {
        cur = cur->next;
    }
    int count = endLine - startLine + 1;
    for (int i = 0; i < count && cur != nullptr; ++i)
    {
        deletedLines.pushBack(cur->data);
        cur = cur->next;
    }

    // Collect bookmarks in the lines about to be deleted
    DynamicArray<Bookmark> removedBookmarks;
    collectBookmarksInLines(ed, startLine, endLine, removedBookmarks);

    // Do the deletion
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

    UndoRedo history;
    if (!history.recordDeleteLines(ed, startLine, deletedLines, removedBookmarks, cb))
        std::cout << "Warning: this change cannot be undone.\n";

    return true;
}