#include "editor/UndoRedo.h"

// Pushes op onto undoStack and clears redoStack. O(1).
bool UndoRedo::record(Editor& ed, const Command& op) {
    if (op.line < 0 || op.col < 0) return false;
    if (!ed.undoStack.push(op)) return false;
    ed.redoStack.clear();
    return true;
}

// Records an insert at (line, col). O(1).
bool UndoRedo::recordInsert(Editor& ed, int line, int col, const std::string& text) {
    Command op;
    op.type = INSERT_TEXT;
    op.line = line;
    op.col = col;
    op.oldText = "";
    op.newText = text;
    return record(ed, op);
}

// Records an erase at (line, col). O(1).
bool UndoRedo::recordErase(Editor& ed, int line, int col, const std::string& erasedText) {
    Command op;
    op.type = DELETE_TEXT;
    op.line = line;
    op.col = col;
    op.oldText = erasedText;
    op.newText = "";
    return record(ed, op);
}

// Records a replace at (line, col). O(1).
bool UndoRedo::recordReplace(Editor& ed, int line, int col,
                             const std::string& oldText, const std::string& newText) {
    Command op;
    op.type = REPLACE;
    op.line = line;
    op.col = col;
    op.oldText = oldText;
    op.newText = newText;
    return record(ed, op);
}

// Reverts the most recent command. O(m).
bool UndoRedo::undo(Editor& ed) {
    if (ed.undoStack.isEmpty() || ed.redoStack.isFull()) return false;

    Command op;
    if (!ed.undoStack.peek(op)) return false;

    Line* line = ed.getLine(op.line);
    if (line == nullptr) return false;

    ed.undoStack.pop(op);

    std::string& document = line->text;

    if (op.type == INSERT_TEXT) {
        // Guard against out-of-range erase.
        if (op.col < 0 || op.col + (int)op.newText.length() > (int)document.length()) {
            ed.undoStack.push(op); // restore state
            return false;
        }
        document.erase(op.col, op.newText.length());
    } else if (op.type == DELETE_TEXT) {
        if (op.col < 0 || op.col > (int)document.length()) {
            ed.undoStack.push(op);
            return false;
        }
        document.insert(op.col, op.oldText);
    } else if (op.type == REPLACE) {
        if (op.col < 0 || op.col + (int)op.newText.length() > (int)document.length()) {
            ed.undoStack.push(op);
            return false;
        }
        document.replace(op.col, op.newText.length(), op.oldText);
    }

    ed.redoStack.push(op);
    ed.setModified(true);
    return true;
}

// Reapplies the most recently undone command. O(m).
bool UndoRedo::redo(Editor& ed) {
    if (ed.redoStack.isEmpty() || ed.undoStack.isFull()) return false;

    Command op;
    if (!ed.redoStack.peek(op)) return false;

    Line* line = ed.getLine(op.line);
    if (line == nullptr) return false;

    ed.redoStack.pop(op);

    std::string& document = line->text;

    if (op.type == INSERT_TEXT) {
        if (op.col < 0 || op.col > (int)document.length()) {
            ed.redoStack.push(op);
            return false;
        }
        document.insert(op.col, op.newText);
    } else if (op.type == DELETE_TEXT) {
        if (op.col < 0 || op.col + (int)op.oldText.length() > (int)document.length()) {
            ed.redoStack.push(op);
            return false;
        }
        document.erase(op.col, op.oldText.length());
    } else if (op.type == REPLACE) {
        if (op.col < 0 || op.col + (int)op.oldText.length() > (int)document.length()) {
            ed.redoStack.push(op);
            return false;
        }
        document.replace(op.col, op.oldText.length(), op.newText);
    }

    ed.undoStack.push(op);
    ed.setModified(true);
    return true;
}

// True if there is at least one command to undo. O(1).
bool UndoRedo::canUndo(const Editor& ed) const {
    return !ed.undoStack.isEmpty();
}

// True if there is at least one command to redo. O(1).
bool UndoRedo::canRedo(const Editor& ed) const {
    return !ed.redoStack.isEmpty();
}

// Clears both history stacks. O(n).
void UndoRedo::clearHistory(Editor& ed) {
    ed.undoStack.clear();
    ed.redoStack.clear();
}