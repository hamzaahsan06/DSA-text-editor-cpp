#ifndef FORMATTING_H
#define FORMATTING_H

#include <string>
#include "editor/Types.h"
#include "ds/DynamicArray.h"
#include "editor/Editor.h"

class Formatting {
private:
    DynamicArray<FormatSpan> spans;

public:
    bool addFormat(int startCol, int endCol, bool isBold, bool isItalic);
    bool removeFormat(int index);
    void clearFormats();
    int count() const;
    std::string applyFormatting(const std::string& lineText) const;

    // Editor-facing API stores spans on each Line, where edits can maintain them.
    bool addFormat(Editor& editor, int line, int startCol, int endCol,
                   bool isBold, bool isItalic) const;
    bool removeFormat(Editor& editor, int line, int formatIndex) const;
    int count(const Editor& editor, int line) const;
    std::string applyFormatting(const Line& line) const;

    // Keep line-local spans aligned with text mutations. onSplitLine expects the
    // text split and new line insertion to have already happened.
    bool onInsertText(Editor& editor, int line, int col, int length) const;
    bool onDeleteText(Editor& editor, int line, int col, int length) const;
    bool onSplitLine(Editor& editor, int line, int splitCol) const;
};

#endif
