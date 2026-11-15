#include "editor/UndoRedo.h"

UndoRedo::Operation::Operation(OperationType t, int p, const std::string& oldT, const std::string& newT)
    : type(t), position(p), oldText(oldT), newText(newT) {}

bool UndoRedo::record(const Operation& op) {
    if (op.position < 0) return false;
    if (!undoStack.push(op)) return false;
    redoStack.clear();
    return true;
}

bool UndoRedo::recordInsert(int position, const std::string& text) {
    return record(Operation(INSERT, position, "", text));
}

bool UndoRedo::recordErase(int position, const std::string& erasedText) {
    return record(Operation(ERASE, position, erasedText, ""));
}

bool UndoRedo::recordReplace(int position, const std::string& oldText, const std::string& newText) {
    return record(Operation(REPLACE, position, oldText, newText));
}

bool UndoRedo::undo(std::string& document) {
    if (undoStack.isEmpty() || redoStack.isFull()) return false;

    Operation op(INSERT, 0, "", "");
    undoStack.pop(op);

    if (op.type == INSERT) {
        document.erase(op.position, op.newText.length());
    } else if (op.type == ERASE) {
        document.insert(op.position, op.oldText);
    } else if (op.type == REPLACE) {
        document.replace(op.position, op.newText.length(), op.oldText);
    }

    redoStack.push(op);
    return true;
}

bool UndoRedo::redo(std::string& document) {
    if (redoStack.isEmpty() || undoStack.isFull()) return false;

    Operation op(INSERT, 0, "", "");
    redoStack.pop(op);

    if (op.type == INSERT) {
        document.insert(op.position, op.newText);
    } else if (op.type == ERASE) {
        document.erase(op.position, op.oldText.length());
    } else if (op.type == REPLACE) {
        document.replace(op.position, op.oldText.length(), op.newText);
    }

    undoStack.push(op);
    return true;
}

bool UndoRedo::canUndo() const {
    return !undoStack.isEmpty();
}

bool UndoRedo::canRedo() const {
    return !redoStack.isEmpty();
}

void UndoRedo::clearHistory() {
    undoStack.clear();
    redoStack.clear();
}