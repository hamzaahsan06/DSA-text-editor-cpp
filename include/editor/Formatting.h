#ifndef FORMATTING_H
#define FORMATTING_H

#include <string>
#include "editor/Types.h"
#include "ds/DynamicArray.h"

class Formatting {
private:
    DynamicArray<FormatSpan> spans;

public:
    bool addFormat(int startCol, int endCol, bool isBold, bool isItalic);
    bool removeFormat(int index);
    void clearFormats();
    int count() const;
    std::string applyFormatting(const std::string& lineText) const;
    static std::string applySpans(const std::string& lineText,
                                  const DynamicArray<FormatSpan>& spans);
};

#endif