#include "editor/Formatting.h"

bool Formatting::addFormat(int startCol, int endCol, bool isBold, bool isItalic) {
    // Half-open [startCol, endCol): empty or negative ranges are invalid.
    if (startCol < 0 || endCol <= startCol) return false;
    if (!isBold && !isItalic) return false;

    FormatSpan span;
    span.startCol = startCol;
    span.endCol = endCol;

    if (isBold && isItalic) {
        span.type = BOLD_ITALIC;
    } else if (isBold) {
        span.type = BOLD;
    } else {
        span.type = ITALIC;
    }

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

std::string Formatting::applySpans(const std::string& lineText,
                                   const DynamicArray<FormatSpan>& spans) {
    if (spans.isEmpty() || lineText.empty()) {
        return lineText;
    }

    std::string result = "";
    int len = static_cast<int>(lineText.length());

    for (int i = 0; i < len; ++i) {
        bool bold = false;
        bool italic = false;

        for (int j = 0; j < spans.size(); ++j) {
            FormatSpan s;
            spans.get(j, s);

            if (i >= s.startCol && i < s.endCol) {
                if (s.type == BOLD || s.type == BOLD_ITALIC) bold = true;
                if (s.type == ITALIC || s.type == BOLD_ITALIC) italic = true;
            }
        }

        if (bold) result += "\033[1m";
        if (italic) result += "\033[3m";
        result += lineText[i];
        if (bold || italic) result += "\033[0m";
    }

    return result;
}

std::string Formatting::applyFormatting(const std::string& lineText) const {
    return applySpans(lineText, spans);
}