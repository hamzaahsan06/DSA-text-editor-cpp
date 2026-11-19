#include "editor/UndoRedo.h"
#include "editor/Bookmarks.h"

static bool buildInsertedText(const std::string &current, int col,
                              const std::string &insertText, std::string &out)
{
    if (col < 0 || col > (int)current.length())
        return false;
    out = current.substr(0, col) + insertText + current.substr(col);
    return true;
}

static bool buildErasedText(const std::string &current, int col,
                            int removeLength, std::string &out)
{
    if (col < 0 || removeLength < 0 || col + removeLength > (int)current.length())
        return false;
    out = current.substr(0, col) + current.substr(col + removeLength);
    return true;
}

static int lineCountOfBlob(const std::string &blob)
{
    int lines = 1;
    for (int i = 0; i < (int)blob.length(); ++i)
    {
        if (blob[i] == '\n')
            ++lines;
    }
    return lines;
}

static void splitBlobIntoLines(const std::string &blob, DynamicArray<std::string> &lines)
{
    int start = 0;
    for (int i = 0; i < (int)blob.length(); ++i)
    {
        if (blob[i] == '\n')
        {
            lines.pushBack(blob.substr(start, i - start));
            start = i + 1;
        }
    }
    lines.pushBack(blob.substr(start));
}

static void splitLineSpans(Line &first, Line &second, int col)
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

static void mergeLineSpans(Line &first, const Line &second, int splitCol)
{
    for (int i = 0; i < second.formats.size(); ++i)
    {
        FormatSpan tail;
        if (!second.formats.get(i, tail))
            continue;

        tail.startCol += splitCol;
        tail.endCol += splitCol;

        bool merged = false;
        if (tail.startCol == splitCol && first.formats.size() > 0)
        {
            int lastIdx = first.formats.size() - 1;
            FormatSpan head;
            if (first.formats.get(lastIdx, head) &&
                head.endCol == splitCol && head.type == tail.type)
            {
                head.endCol = tail.endCol;
                first.formats.set(lastIdx, head);
                merged = true;
            }
        }

        if (!merged)
        {
            first.formats.pushBack(tail);
        }
    }
}

static void clampCursorToDocument(Editor &ed, int targetLine, int targetCol)
{
    int count = ed.getLineCount();
    if (count <= 0)
    {
        ed.cursorLine = 0;
        ed.cursorCol = 0;
        return;
    }
    if (targetLine < 0)
        targetLine = 0;
    if (targetLine >= count)
        targetLine = count - 1;

    Line *line = ed.getLine(targetLine);
    int maxCol = (line != nullptr) ? (int)line->text.length() : 0;
    if (targetCol < 0)
        targetCol = 0;
    if (targetCol > maxCol)
        targetCol = maxCol;

    ed.cursorLine = targetLine;
    ed.cursorCol = targetCol;
}

bool UndoRedo::record(Editor &ed, const Command &op)
{
    if (op.line < 0 || op.col < 0)
        return false;
    if (ed.getLine(op.line) == nullptr)
        return false;
    if (!ed.undoStack.push(op))
        return false;
    ed.redoStack.clear();
    return true;
}

bool UndoRedo::recordInsert(Editor &ed, int line, int col, const std::string &text)
{
    Command op;
    op.type = INSERT_TEXT;
    op.line = line;
    op.col = col;
    op.oldText = "";
    op.newText = text;
    return record(ed, op);
}

bool UndoRedo::recordErase(Editor &ed, int line, int col, const std::string &erasedText)
{
    Command op;
    op.type = DELETE_TEXT;
    op.line = line;
    op.col = col;
    op.oldText = erasedText;
    op.newText = "";
    return record(ed, op);
}

bool UndoRedo::recordReplace(Editor &ed, int line, int col,
                             const std::string &oldText, const std::string &newText)
{
    Command op;
    op.type = REPLACE;
    op.line = line;
    op.col = col;
    op.oldText = oldText;
    op.newText = newText;
    return record(ed, op);
}

bool UndoRedo::recordInsertLine(Editor &ed, int line, const std::string &text)
{
    if (line < 0 || line > ed.getLineCount())
        return false;
    Command op;
    op.type = INSERT_LINE;
    op.line = line;
    op.col = 0;
    op.oldText = "";
    op.newText = text;
    if (!ed.undoStack.push(op))
        return false;
    ed.redoStack.clear();
    return true;
}

bool UndoRedo::recordDeleteLine(Editor &ed, int line, const std::string &oldText)
{
    if (line < 0 || line >= ed.getLineCount())
        return false;
    Command op;
    op.type = DELETE_LINE;
    op.line = line;
    op.col = 0;
    op.oldText = oldText;
    op.newText = "";
    if (!ed.undoStack.push(op))
        return false;
    ed.redoStack.clear();
    return true;
}

static bool applyUndo(Editor &ed, const Command &op)
{
    Line *line = ed.getLine(op.line);

    if (op.type == INSERT_TEXT || op.type == DELETE_TEXT || op.type == REPLACE)
    {
        if (line == nullptr)
            return false;
        std::string updated;
        int cursorAfter = op.col;
        if (op.type == INSERT_TEXT)
        {
            if (!buildErasedText(line->text, op.col, (int)op.newText.length(), updated))
                return false;
            cursorAfter = op.col;
        }
        else if (op.type == DELETE_TEXT)
        {
            if (!buildInsertedText(line->text, op.col, op.oldText, updated))
                return false;
            cursorAfter = op.col + (int)op.oldText.length();
        }
        else
        {
            std::string middle;
            if (!buildErasedText(line->text, op.col, (int)op.newText.length(), middle))
                return false;
            if (!buildInsertedText(middle, op.col, op.oldText, updated))
                return false;
            cursorAfter = op.col + (int)op.oldText.length();
        }
        line->text = updated;
        clampCursorToDocument(ed, op.line, cursorAfter);
        return true;
    }

    if (op.type == INSERT_LINE)
    {
        bool isLineSplit = !op.newText.empty() || op.oldText == "\n";
        if (isLineSplit)
        {
            if (line == nullptr)
                return false;
            Line *nextLine = ed.getLine(op.line + 1);
            if (nextLine != nullptr)
            {
                mergeLineSpans(*line, *nextLine, op.col);
                line->text = op.newText;
                Line dummy;
                ed.document.removeAtPosition(op.line + 1, dummy);
                adjustBookmarksOnLineDelete(ed, op.line + 1);
            }
            clampCursorToDocument(ed, op.line, op.col);
            return true;
        }

        if (line == nullptr)
            return false;
        Line removed;
        if (!ed.document.removeAtPosition(op.line, removed))
            return false;
        adjustBookmarksOnLineDelete(ed, op.line);
        clampCursorToDocument(ed, op.line, 0);
        return true;
    }

    if (op.type == DELETE_LINE)
    {
        DynamicArray<std::string> pieces;
        splitBlobIntoLines(op.oldText, pieces);
        for (int i = 0; i < pieces.size(); ++i)
        {
            Line restored;
            pieces.get(i, restored.text);
            if (!ed.document.insertAtPosition(op.line + i, restored))
                return false;
            adjustBookmarksOnLineInsert(ed, op.line + i);
        }
        clampCursorToDocument(ed, op.line, 0);
        return true;
    }

    return false;
}

static bool applyRedo(Editor &ed, const Command &op)
{
    Line *line = ed.getLine(op.line);

    if (op.type == INSERT_TEXT || op.type == DELETE_TEXT || op.type == REPLACE)
    {
        if (line == nullptr)
            return false;
        std::string updated;
        int cursorAfter = op.col;
        if (op.type == INSERT_TEXT)
        {
            if (!buildInsertedText(line->text, op.col, op.newText, updated))
                return false;
            cursorAfter = op.col + (int)op.newText.length();
        }
        else if (op.type == DELETE_TEXT)
        {
            if (!buildErasedText(line->text, op.col, (int)op.oldText.length(), updated))
                return false;
            cursorAfter = op.col;
        }
        else
        {
            std::string middle;
            if (!buildErasedText(line->text, op.col, (int)op.oldText.length(), middle))
                return false;
            if (!buildInsertedText(middle, op.col, op.newText, updated))
                return false;
            cursorAfter = op.col + (int)op.newText.length();
        }
        line->text = updated;
        clampCursorToDocument(ed, op.line, cursorAfter);
        return true;
    }

    if (op.type == INSERT_LINE)
    {
        bool isLineSplit = !op.newText.empty() || op.oldText == "\n";
        if (isLineSplit)
        {
            if (line == nullptr)
                return false;
            Line below;
            below.text = line->text.substr(op.col);
            line->text.erase(op.col);
            splitLineSpans(*line, below, op.col);
            if (!ed.document.insertAtPosition(op.line + 1, below))
                return false;
            adjustBookmarksOnLineInsert(ed, op.line + 1);
            clampCursorToDocument(ed, op.line + 1, 0);
            return true;
        }

        Line fresh;
        fresh.text = op.newText;
        if (!ed.document.insertAtPosition(op.line, fresh))
            return false;
        adjustBookmarksOnLineInsert(ed, op.line);
        clampCursorToDocument(ed, op.line, 0);
        return true;
    }

    if (op.type == DELETE_LINE)
    {
        int removeCount = lineCountOfBlob(op.oldText);
        for (int i = 0; i < removeCount; ++i)
        {
            if (ed.getLine(op.line) == nullptr)
                return false;
            Line removed;
            if (!ed.document.removeAtPosition(op.line, removed))
                return false;
            adjustBookmarksOnLineDelete(ed, op.line);
        }
        clampCursorToDocument(ed, op.line, 0);
        return true;
    }

    return false;
}

bool UndoRedo::undo(Editor &ed)
{
    Command op;
    if (!ed.undoStack.peek(op))
        return false;
    if (!applyUndo(ed, op))
        return false;
    ed.undoStack.pop(op);
    ed.redoStack.push(op);
    ed.setModified(true);
    return true;
}

bool UndoRedo::redo(Editor &ed)
{
    Command op;
    if (!ed.redoStack.peek(op))
        return false;
    if (!applyRedo(ed, op))
        return false;
    ed.redoStack.pop(op);
    ed.undoStack.push(op);
    ed.setModified(true);
    return true;
}

bool UndoRedo::canUndo(const Editor &ed) const
{
    return !ed.undoStack.isEmpty();
}

bool UndoRedo::canRedo(const Editor &ed) const
{
    return !ed.redoStack.isEmpty();
}

void UndoRedo::clearHistory(Editor &ed)
{
    ed.undoStack.clear();
    ed.redoStack.clear();
}