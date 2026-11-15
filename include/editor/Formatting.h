#ifndef FORMATTING_H
#define FORMATTING_H

#include <string>
#include "ds/DynamicArray.h"

class Formatting {
private:
    struct FormatSpan {
        int startCol;
        int endCol;
        bool isBold;
        bool isItalic;

        FormatSpan();
        FormatSpan(int start, int end, bool bold, bool italic);
    };

    DynamicArray<FormatSpan> spans;

public:
    bool addFormat(int startCol, int endCol, bool isBold, bool isItalic);
    bool removeFormat(int index);
    void clearFormats();
    int count() const;
    std::string applyFormatting(const std::string& lineText) const;
};

#endif