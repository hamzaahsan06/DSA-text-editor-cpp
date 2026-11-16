#include "editor/UndoRedo.h"

bool UndoRedo::record(Editor& ed, const Command& op) {
    if (op.col < 0) return false;
    if (!ed.undoStack.push(op)) return false;
    ed.redoStack.clear();
    return true;
}

bool UndoRedo::recordInsert(Editor& ed, int position, const std::string& text) {
    Command op;
    op.type = INSERT_TEXT;
    op.line = 0;
    op.col = position;
    op.oldText = "";
    op.newText = text;

    // NEEDS: A line parameter or position-to-line/column mapping
    // to support insertions beyond the first line.
    return record(ed, op);
}

bool UndoRedo::recordErase(Editor& ed, int position, const std::string& erasedText) {
    Command op;
    op.type = DELETE_TEXT;
    op.line = 0;
    op.col = position;
    op.oldText = erasedText;
    op.newText = "";

    // NEEDS: A line parameter or position-to-line/column mapping
    // to support deletions beyond the first line.
    return record(ed, op);
}

bool UndoRedo::recordReplace(Editor& ed, int position, const std::string& oldText, const std::string& newText) {
    Command op;
    op.type = REPLACE;
    op.line = 0;
    op.col = position;
    op.oldText = oldText;
    op.newText = newText;

    // NEEDS: A line parameter or position-to-line/column mapping
    // to support replacements beyond the first line.
    return record(ed, op);
}

bool UndoRedo::undo(Editor& ed) {
    if (ed.undoStack.isEmpty() || ed.redoStack.isFull()) return false;

    Command op;
    if (!ed.undoStack.peek(op)) return false;

    Line* line = ed.getLine(op.line);
    if (line == nullptr) return false;

    ed.undoStack.pop(op);

    std::string& document = line->text;

    if (op.type == INSERT_TEXT) {
        document.erase(op.col, op.newText.length());
    } else if (op.type == DELETE_TEXT) {
        document.insert(op.col, op.oldText);
    } else if (op.type == REPLACE) {
        document.replace(op.col, op.newText.length(), op.oldText);
    }

    ed.redoStack.push(op);
    return true;
}

bool UndoRedo::redo(Editor& ed) {
    if (ed.redoStack.isEmpty() || ed.undoStack.isFull()) return false;

    Command op;
    if (!ed.redoStack.peek(op)) return false;

    Line* line = ed.getLine(op.line);
    if (line == nullptr) return false;

    ed.redoStack.pop(op);

    std::string& document = line->text;

    if (op.type == INSERT_TEXT) {
        document.insert(op.col, op.newText);
    } else if (op.type == DELETE_TEXT) {
        document.erase(op.col, op.oldText.length());
    } else if (op.type == REPLACE) {
        document.replace(op.col, op.oldText.length(), op.newText);
    }

    ed.undoStack.push(op);
    return true;
}

bool UndoRedo::canUndo(const Editor& ed) const {
    return !ed.undoStack.isEmpty();
}

bool UndoRedo::canRedo(const Editor& ed) const {
    return !ed.redoStack.isEmpty();
}

void UndoRedo::clearHistory(Editor& ed) {
    ed.undoStack.clear();
    ed.redoStack.clear();
}