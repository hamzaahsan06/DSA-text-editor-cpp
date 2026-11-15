#include "editor/Formatting.h"

Formatting::FormatSpan::FormatSpan() {
    startCol = 0;
    endCol = 0;
    isBold = false;
    isItalic = false;
}

Formatting::FormatSpan::FormatSpan(int start, int end, bool bold, bool italic) {
    startCol = start;
    endCol = end;
    isBold = bold;
    isItalic = italic;
}

bool Formatting::addFormat(int startCol, int endCol, bool isBold, bool isItalic) {
    if (startCol < 0 || endCol < startCol) return false;
    if (!isBold && !isItalic) return false;
    FormatSpan span(startCol, endCol, isBold, isItalic);
    return spans.pushBack(span);
}

bool Formatting::removeFormat(int index) {
    FormatSpan removed;
    return spans.removeAt(index, removed);
}

void Formatting::clearFormats() {
    spans.clear();
}

int Formatting::count() const {
    return spans.size();
}

std::string Formatting::applyFormatting(const std::string& lineText) const {
    std::string result = "";
    int len = static_cast<int>(lineText.length());

    for (int i = 0; i < len; ++i) {
        bool bold = false;
        bool italic = false;

        for (int j = 0; j < spans.size(); ++j) {
            FormatSpan s;
            spans.get(j, s);
            if (i >= s.startCol && i <= s.endCol) {
                if (s.isBold) bold = true;
                if (s.isItalic) italic = true;
            }
        }

        if (bold) result += "\033[1m";
        if (italic) result += "\033[3m";
        result += lineText[i];
        if (bold || italic) result += "\033[0m";
    }

    return result;
}