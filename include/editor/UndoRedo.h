#ifndef UNDO_REDO_H
#define UNDO_REDO_H

#include <string>
#include "editor/Types.h"
#include "editor/Editor.h"

// Records undoable commands and performs undo/redo on the document.
class UndoRedo
{
public:
    // Generic validated recorder: pushes op onto undoStack, clears redoStack. O(1).
    bool record(Editor &ed, const Command &op);

    // Records an insert at (line, col). O(1).
    bool recordInsert(Editor &ed, int line, int col, const std::string &text);

    // Records an erase at (line, col). O(1).
    bool recordErase(Editor &ed, int line, int col, const std::string &erasedText);

    // Records a replace at (line, col). O(1).
    bool recordReplace(Editor &ed, int line, int col,
                       const std::string &oldText, const std::string &newText);

    // Records a complete line replacement as one undoable operation.
    bool recordReplaceLine(Editor &ed, int line,
                           const std::string &oldText, const std::string &newText);

    // Reverts the most recent command. O(m) where m = text touched.
    bool undo(Editor &ed);

    // Reapplies the most recently undone command. O(m).
    bool redo(Editor &ed);

    bool canUndo(const Editor &ed) const;
    bool canRedo(const Editor &ed) const;
    void clearHistory(Editor &ed);
};

#endif
