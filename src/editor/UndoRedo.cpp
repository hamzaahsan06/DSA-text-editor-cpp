#include "editor/UndoRedo.h"
#include "editor/Bookmarks.h"

// Helper: safely clamps the editor's cursor to the current document state.
static void clampCursor(Editor &ed)
{
    if (ed.document.isEmpty())
    {
        ed.cursorLine = 0;
        ed.cursorCol = 0;
        return;
    }
    if (ed.cursorLine < 0)
        ed.cursorLine = 0;
    if (ed.cursorLine >= ed.getLineCount())
        ed.cursorLine = ed.getLineCount() - 1;

    Line *line = ed.getLine(ed.cursorLine);
    if (line != nullptr)
    {
        if (ed.cursorCol < 0)
            ed.cursorCol = 0;
        if (ed.cursorCol > (int)line->text.length())
            ed.cursorCol = (int)line->text.length();
    }
}

// Helper: safely pushes a command and clears redo.
static bool pushCommand(Editor &ed, const Command &cmd)
{
    if (!ed.undoStack.push(cmd))
        return false;
    ed.redoStack.clear();
    return true;
}

// Records a single-line in-place edit.
bool UndoRedo::recordLineEdit(Editor &ed, OpType type, int line, const Line &before, Position cursorBefore)
{
    if (line < 0 || line >= ed.getLineCount())
        return false;
    Line *afterLine = ed.getLine(line);
    if (afterLine == nullptr)
        return false;

    Command cmd;
    cmd.type = type;
    cmd.startLine = line;
    cmd.before.pushBack(before);
    cmd.after.pushBack(*afterLine);
    cmd.cursorBefore = cursorBefore;
    cmd.cursorAfter.line = ed.cursorLine;
    cmd.cursorAfter.col = ed.cursorCol;

    return pushCommand(ed, cmd);
}

// Records a block of lines being replaced by a new block.
bool UndoRedo::recordBlockEdit(Editor &ed, OpType type, int startLine, const DynamicArray<Line> &before, int afterCount, Position cursorBefore)
{
    if (startLine < 0 || startLine > ed.getLineCount())
        return false;
    if (afterCount < 0 || startLine + afterCount > ed.getLineCount())
        return false;

    Command cmd;
    cmd.type = type;
    cmd.startLine = startLine;

    for (int i = 0; i < before.size(); ++i)
    {
        Line l;
        before.get(i, l);
        cmd.before.pushBack(l);
    }

    for (int i = 0; i < afterCount; ++i)
    {
        Line *l = ed.getLine(startLine + i);
        if (l != nullptr)
            cmd.after.pushBack(*l);
    }

    cmd.cursorBefore = cursorBefore;
    cmd.cursorAfter.line = ed.cursorLine;
    cmd.cursorAfter.col = ed.cursorCol;

    return pushCommand(ed, cmd);
}

// Records a line being split into two.
bool UndoRedo::recordSplit(Editor &ed, int line, int col, const Line &beforeLine, Position cursorBefore)
{
    if (line < 0 || line >= ed.getLineCount())
        return false;
    if (line + 1 >= ed.getLineCount())
        return false; // Must have 2 lines to capture 'after'

    Command cmd;
    cmd.type = SPLIT_LINE;
    cmd.startLine = line;
    cmd.splitCol = col;
    cmd.before.pushBack(beforeLine);

    Line *l1 = ed.getLine(line);
    Line *l2 = ed.getLine(line + 1);
    if (l1)
        cmd.after.pushBack(*l1);
    if (l2)
        cmd.after.pushBack(*l2);

    cmd.cursorBefore = cursorBefore;
    cmd.cursorAfter.line = ed.cursorLine;
    cmd.cursorAfter.col = ed.cursorCol;

    return pushCommand(ed, cmd);
}

// Records a block of lines being deleted.
bool UndoRedo::recordDeleteLines(Editor &ed, int startLine, const DynamicArray<Line> &deletedLines, const DynamicArray<Bookmark> &removedBookmarks, Position cursorBefore)
{
    if (startLine < 0 || startLine > ed.getLineCount())
        return false;

    Command cmd;
    cmd.type = DELETE_LINES;
    cmd.startLine = startLine;

    for (int i = 0; i < deletedLines.size(); ++i)
    {
        Line l;
        deletedLines.get(i, l);
        cmd.before.pushBack(l);
    }

    for (int i = 0; i < removedBookmarks.size(); ++i)
    {
        Bookmark b;
        removedBookmarks.get(i, b);
        cmd.removedBookmarks.pushBack(b);
    }

    cmd.cursorBefore = cursorBefore;
    cmd.cursorAfter.line = ed.cursorLine;
    cmd.cursorAfter.col = ed.cursorCol;

    return pushCommand(ed, cmd);
}

// Reverts the most recent command.
bool UndoRedo::undo(Editor &ed)
{
    Command op;
    if (!ed.undoStack.peek(op))
        return false;

    // Validate that we have enough lines to remove the "after" block
    if (op.startLine < 0 || op.startLine > ed.getLineCount())
        return false;
    if (op.startLine + op.after.size() > ed.getLineCount())
        return false;

    // Remove the "after" block
    for (int i = 0; i < op.after.size(); ++i)
    {
        Line dummy;
        ed.document.removeAtPosition(op.startLine, dummy);
    }

    // Insert the "before" block
    for (int i = 0; i < op.before.size(); ++i)
    {
        Line l;
        op.before.get(i, l);
        ed.document.insertAtPosition(op.startLine + i, l);
    }

    // Handle Bookmarks
    if (op.type == SPLIT_LINE)
    {
        adjustBookmarksOnJoin(ed, op.startLine, op.splitCol);
    }
    else if (op.type == DELETE_LINES)
    {
        for (int i = 0; i < op.before.size(); ++i)
        {
            adjustBookmarksOnLineInsert(ed, op.startLine);
        }
        restoreBookmarks(ed, op.removedBookmarks);
    }

    // Move command to redoStack
    ed.undoStack.pop(op);
    ed.redoStack.push(op);

    // Restore Cursor
    ed.cursorLine = op.cursorBefore.line;
    ed.cursorCol = op.cursorBefore.col;
    clampCursor(ed);

    ed.setModified(true);
    return true;
}

// Reapplies the most recently undone command.
bool UndoRedo::redo(Editor &ed)
{
    Command op;
    if (!ed.redoStack.peek(op))
        return false;

    // Validate that we have enough lines to remove the "before" block
    if (op.startLine < 0 || op.startLine > ed.getLineCount())
        return false;
    if (op.startLine + op.before.size() > ed.getLineCount())
        return false;

    // Remove the "before" block
    for (int i = 0; i < op.before.size(); ++i)
    {
        Line dummy;
        ed.document.removeAtPosition(op.startLine, dummy);
    }

    // Insert the "after" block
    for (int i = 0; i < op.after.size(); ++i)
    {
        Line l;
        op.after.get(i, l);
        ed.document.insertAtPosition(op.startLine + i, l);
    }

    // Handle Bookmarks
    if (op.type == SPLIT_LINE)
    {
        adjustBookmarksOnSplit(ed, op.startLine, op.splitCol);
    }
    else if (op.type == DELETE_LINES)
    {
        for (int i = 0; i < op.before.size(); ++i)
        {
            adjustBookmarksOnLineDelete(ed, op.startLine);
        }
    }

    // Move command to undoStack
    ed.redoStack.pop(op);
    ed.undoStack.push(op);

    // Restore Cursor
    ed.cursorLine = op.cursorAfter.line;
    ed.cursorCol = op.cursorAfter.col;
    clampCursor(ed);

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