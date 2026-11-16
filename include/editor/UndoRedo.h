#ifndef UNDO_REDO_H
#define UNDO_REDO_H

#include <string>
#include "editor/Types.h"
#include "editor/Editor.h"

// Basis for (insert/erase/replace) all three operation(one function) .
class UndoRedo {
private:
    bool record(Editor& ed, const Command& op);

public:
    bool recordInsert(Editor& ed, int position, const std::string& text);
    bool recordErase(Editor& ed, int position, const std::string& erasedText);
    bool recordReplace(Editor& ed, int position, const std::string& oldText, const std::string& newText);

    bool undo(Editor& ed);
    bool redo(Editor& ed);

    bool canUndo(const Editor& ed) const;
    bool canRedo(const Editor& ed) const;
    void clearHistory(Editor& ed);
};

#endif