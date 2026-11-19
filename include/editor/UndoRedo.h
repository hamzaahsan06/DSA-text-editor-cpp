#ifndef UNDO_REDO_H
#define UNDO_REDO_H

#include <string>
#include "editor/Types.h"
#include "editor/Editor.h"

// Manages undo/redo history using snapshots of blocks of lines.
class UndoRedo
{
public:
    // Records a single-line in-place edit.
    bool recordLineEdit(Editor &ed, OpType type, int line, const Line &before, Position cursorBefore);

    // Records a block of lines being replaced by a new block.
    bool recordBlockEdit(Editor &ed, OpType type, int startLine, const DynamicArray<Line> &before, int afterCount, Position cursorBefore);

    // Records a line being split into two.
    bool recordSplit(Editor &ed, int line, int col, const Line &beforeLine, Position cursorBefore);

    // Records a block of lines being deleted.
    bool recordDeleteLines(Editor &ed, int startLine, const DynamicArray<Line> &deletedLines, const DynamicArray<Bookmark> &removedBookmarks, Position cursorBefore);

    // Reverts the most recent command.
    bool undo(Editor &ed);

    // Reapplies the most recently undone command.
    bool redo(Editor &ed);

    // True if there is at least one command to undo.
    bool canUndo(const Editor &ed) const;

    // True if there is at least one command to redo.
    bool canRedo(const Editor &ed) const;

    // Clears both history stacks.
    void clearHistory(Editor &ed);
};

#endif