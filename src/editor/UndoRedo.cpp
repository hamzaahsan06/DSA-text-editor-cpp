#include "editor/UndoRedo.h"


// Builds the text that results from inserting insertText into current
// at position col. Returns false if col is out of range.
static bool buildInsertedText(const std::string &current, int col,
                              const std::string &insertText, std::string &out)
{
    if (col < 0 || col > (int)current.length())
    {
        return false;
    }
    out = current.substr(0, col) + insertText + current.substr(col);
    return true;
}

// Builds the text that results from erasing removeLength characters at
// position col. Returns false if the range is out of bounds.
static bool buildErasedText(const std::string &current, int col,
                            int removeLength, std::string &out)
{
    if (col < 0 || removeLength < 0)
    {
        return false;
    }
    if (col + removeLength > (int)current.length())
    {
        return false;
    }
    out = current.substr(0, col) + current.substr(col + removeLength);
    return true;
}

// Counts how many lines are described by a blob saved with '\n' between
// them. An empty blob still represents one (empty) line.
static int lineCountOfBlob(const std::string &blob)
{
    int lines = 1;
    for (int i = 0; i < (int)blob.length(); ++i)
    {
        if (blob[i] == '\n')
        {
            ++lines;
        }
    }
    return lines;
}

// Rebuilds the individual line texts from a blob saved with '\n' between
// them. The result array is appended in order.
static void splitBlobIntoLines(const std::string &blob,
                               DynamicArray<std::string> &lines)
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

// Records op after checking that line and col are non-negative and that
// the target line exists in the document.
bool UndoRedo::record(Editor &ed, const Command &op)
{
    if (op.line < 0 || op.col < 0)
    {
        return false;
    }
    if (ed.getLine(op.line) == nullptr)
    {
        return false;
    }
    if (!ed.undoStack.push(op))
    {
        return false;
    }
    ed.redoStack.clear();
    return true;
}

// Records an INSERT_TEXT command.
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

// Records a DELETE_TEXT command, storing the erased text for undo.
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

// Records a REPLACE command, storing both old and new text.
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

// Records an INSERT_LINE command at the given line index.
bool UndoRedo::recordInsertLine(Editor &ed, int line, const std::string &text)
{
    if (line < 0 || line > ed.getLineCount())
    {
        return false;
    }
    Command op;
    op.type = INSERT_LINE;
    op.line = line;
    op.col = 0;
    op.oldText = "";
    op.newText = text;
    if (!ed.undoStack.push(op))
    {
        return false;
    }
    ed.redoStack.clear();
    return true;
}

// Records a DELETE_LINE command, keeping the line text for undo.
bool UndoRedo::recordDeleteLine(Editor &ed, int line, const std::string &oldText)
{
    if (line < 0 || line >= ed.getLineCount())
    {
        return false;
    }
    Command op;
    op.type = DELETE_LINE;
    op.line = line;
    op.col = 0;
    op.oldText = oldText;
    op.newText = "";
    if (!ed.undoStack.push(op))
    {
        return false;
    }
    ed.redoStack.clear();
    return true;
}

// Applies the inverse of op to the document. Returns false without
// touching the stacks if the op cannot be reversed safely.
static bool applyUndo(Editor &ed, const Command &op)
{
    Line *line = ed.getLine(op.line);

    if (op.type == INSERT_TEXT || op.type == DELETE_TEXT || op.type == REPLACE)
    {
        if (line == nullptr)
        {
            return false;
        }
        std::string updated;
        if (op.type == INSERT_TEXT)
        {
            // Undo an insert by removing exactly the inserted text.
            if (!buildErasedText(line->text, op.col, (int)op.newText.length(), updated))
            {
                return false;
            }
        }
        else if (op.type == DELETE_TEXT)
        {
            // Undo a delete by re-inserting the erased text.
            if (!buildInsertedText(line->text, op.col, op.oldText, updated))
            {
                return false;
            }
        }
        else
        {
            // Undo a replace by removing newText and putting oldText back.
            std::string middle;
            if (!buildErasedText(line->text, op.col, (int)op.newText.length(), middle))
            {
                return false;
            }
            if (!buildInsertedText(middle, op.col, op.oldText, updated))
            {
                return false;
            }
        }
        line->text = updated;
        return true;
    }

    if (op.type == INSERT_LINE)
    {
        // Undo an insert by deleting the line at op.line.
        if (line == nullptr)
        {
            return false;
        }
        Line removed;
        return ed.document.removeAtPosition(op.line, removed);
    }

    if (op.type == DELETE_LINE)
    {
        // Undo a delete by re-inserting the saved line text. When the
        // delete covered several lines, oldText holds them joined by '\n';
        // split it and re-insert each line at the right index.
        DynamicArray<std::string> pieces;
        splitBlobIntoLines(op.oldText, pieces);
        for (int i = 0; i < pieces.size(); ++i)
        {
            Line restored;
            pieces.get(i, restored.text);
            if (!ed.document.insertAtPosition(op.line + i, restored))
            {
                return false;
            }
        }
        return true;
    }

    // FORMAT_APPLY / FORMAT_REMOVE are not reversible with current
    // Command fields; refuse so the stacks stay consistent.
    return false;
}

// Applies op forward to the document. Used by redo.
static bool applyRedo(Editor &ed, const Command &op)
{
    Line *line = ed.getLine(op.line);

    if (op.type == INSERT_TEXT || op.type == DELETE_TEXT || op.type == REPLACE)
    {
        if (line == nullptr)
        {
            return false;
        }
        std::string updated;
        if (op.type == INSERT_TEXT)
        {
            if (!buildInsertedText(line->text, op.col, op.newText, updated))
            {
                return false;
            }
        }
        else if (op.type == DELETE_TEXT)
        {
            if (!buildErasedText(line->text, op.col, (int)op.oldText.length(), updated))
            {
                return false;
            }
        }
        else
        {
            std::string middle;
            if (!buildErasedText(line->text, op.col, (int)op.oldText.length(), middle))
            {
                return false;
            }
            if (!buildInsertedText(middle, op.col, op.newText, updated))
            {
                return false;
            }
        }
        line->text = updated;
        return true;
    }

    if (op.type == INSERT_LINE)
    {
        Line fresh;
        fresh.text = op.newText;
        return ed.document.insertAtPosition(op.line, fresh);
    }

    if (op.type == DELETE_LINE)
    {
        // Redo a delete: remove as many lines as the saved blob describes.
        int removeCount = lineCountOfBlob(op.oldText);
        for (int i = 0; i < removeCount; ++i)
        {
            if (ed.getLine(op.line) == nullptr)
            {
                return false;
            }
            Line removed;
            if (!ed.document.removeAtPosition(op.line, removed))
            {
                return false;
            }
        }
        return true;
    }

    return false;
}

// Reverts the most recent command. On success, moves it to redoStack.
// On failure, leaves both stacks untouched.
bool UndoRedo::undo(Editor &ed)
{
    Command op;
    if (!ed.undoStack.peek(op))
    {
        return false;
    }
    if (!applyUndo(ed, op))
    {
        return false;
    }
    ed.undoStack.pop(op);
    ed.redoStack.push(op);
    ed.setModified(true);
    return true;
}

// Reapplies the most recently undone command. On success, moves it back
// to undoStack. On failure, leaves both stacks untouched.
bool UndoRedo::redo(Editor &ed)
{
    Command op;
    if (!ed.redoStack.peek(op))
    {
        return false;
    }
    if (!applyRedo(ed, op))
    {
        return false;
    }
    ed.redoStack.pop(op);
    ed.undoStack.push(op);
    ed.setModified(true);
    return true;
}

// True if there is at least one command to undo.
bool UndoRedo::canUndo(const Editor &ed) const
{
    return !ed.undoStack.isEmpty();
}

// True if there is at least one command to redo.
bool UndoRedo::canRedo(const Editor &ed) const
{
    return !ed.redoStack.isEmpty();
}

// Clears both history stacks.
void UndoRedo::clearHistory(Editor &ed)
{
    ed.undoStack.clear();
    ed.redoStack.clear();
}