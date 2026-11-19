#include "editor/UndoRedo.h"

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
        if (op.type == INSERT_TEXT)
        {
            if (!buildErasedText(line->text, op.col, (int)op.newText.length(), updated))
                return false;
        }
        else if (op.type == DELETE_TEXT)
        {
            if (!buildInsertedText(line->text, op.col, op.oldText, updated))
                return false;
        }
        else
        {
            std::string middle;
            if (!buildErasedText(line->text, op.col, (int)op.newText.length(), middle))
                return false;
            if (!buildInsertedText(middle, op.col, op.oldText, updated))
                return false;
        }
        line->text = updated;
        return true;
    }

    if (op.type == INSERT_LINE)
    {
        // If newText is not empty, this was a line split (TextOps::insertNewLine).
        // We must merge the two lines back together.
        if (!op.newText.empty())
        {
            if (line == nullptr)
                return false;
            Line *nextLine = ed.getLine(op.line + 1);
            if (nextLine != nullptr)
            {
                line->text = op.newText; // Restore the merged text
                Line dummy;
                ed.document.removeAtPosition(op.line + 1, dummy);
            }
            return true;
        }
        // Normal empty line insert undo
        if (line == nullptr)
            return false;
        Line removed;
        return ed.document.removeAtPosition(op.line, removed);
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
        }
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
        if (op.type == INSERT_TEXT)
        {
            if (!buildInsertedText(line->text, op.col, op.newText, updated))
                return false;
        }
        else if (op.type == DELETE_TEXT)
        {
            if (!buildErasedText(line->text, op.col, (int)op.oldText.length(), updated))
                return false;
        }
        else
        {
            std::string middle;
            if (!buildErasedText(line->text, op.col, (int)op.oldText.length(), middle))
                return false;
            if (!buildInsertedText(middle, op.col, op.newText, updated))
                return false;
        }
        line->text = updated;
        return true;
    }

    if (op.type == INSERT_LINE)
    {
        // If newText is not empty, it was a split. Re-split it.
        if (!op.newText.empty())
        {
            if (line == nullptr)
                return false;
            Line below;
            below.text = line->text.substr(op.col);
            line->text.erase(op.col);
            ed.document.insertAtPosition(op.line + 1, below);
            return true;
        }
        Line fresh;
        fresh.text = op.newText;
        return ed.document.insertAtPosition(op.line, fresh);
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
        }
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