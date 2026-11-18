#ifndef UNDO_REDO_H
#define UNDO_REDO_H

#include <string>
#include "editor/Types.h"
#include "editor/Editor.h"

// Records undoable commands and performs undo/redo on the document.
// Each successful edit pushes one Command onto undoStack and clears
// redoStack. Undo moves the top command to redoStack; redo moves it back.
class UndoRedo
{
public:
    // Pushes op onto undoStack and clears redoStack.
    bool record(Editor &ed, const Command &op);

    // Records an insert at (line, col).
    bool recordInsert(Editor &ed, int line, int col, const std::string &text);

    // Records an erase at (line, col).
    bool recordErase(Editor &ed, int line, int col, const std::string &erasedText);

    // Records a replace at (line, col).
    bool recordReplace(Editor &ed, int line, int col,
                       const std::string &oldText, const std::string &newText);

    // Records inserting a new line at line index, using the given text.
    bool recordInsertLine(Editor &ed, int line, const std::string &text);

    // Records deleting the line at line index, keeping its old text.
    bool recordDeleteLine(Editor &ed, int line, const std::string &oldText);

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