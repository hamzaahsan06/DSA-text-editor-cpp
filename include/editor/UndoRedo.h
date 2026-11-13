#ifndef UNDO_REDO_H
#define UNDO_REDO_H

#include <string>
#include "ds/Stack.h"
// Basis for (insert/erase/replace) all three operation(one function) .
class UndoRedo {
private:
    enum OperationType { INSERT, ERASE, REPLACE };

    struct Operation {
        OperationType type;
        int position;
        std::string oldText;
        std::string newText;

        Operation(OperationType t, int p, const std::string& oldT, const std::string& newT);
    };

    Stack<Operation> undoStack;
    Stack<Operation> redoStack;

    bool record(const Operation& op);

public:
    bool recordInsert(int position, const std::string& text);
    bool recordErase(int position, const std::string& erasedText);
    bool recordReplace(int position, const std::string& oldText, const std::string& newText);

    bool undo(std::string& document);
    bool redo(std::string& document);

    bool canUndo() const;
    bool canRedo() const;
    void clearHistory();
};

#endif