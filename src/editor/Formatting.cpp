#include "editor/Formatting.h"
#include "editor/Validation.h"

bool Formatting::addFormat(int startCol, int endCol, bool isBold, bool isItalic) {
    if (startCol < 0 || endCol < startCol) return false;
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

std::string Formatting::applyFormatting(const std::string& lineText) const {
    std::string result = "";
    int len = static_cast<int>(lineText.length());

    for (int i = 0; i < len; ++i) {
        bool bold = false;
        bool italic = false;

        for (int j = 0; j < spans.size(); ++j) {
            FormatSpan s;
            spans.get(j, s);

            // FormatSpan ranges are half-open: [startCol, endCol).
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

bool Formatting::addFormat(Editor& editor, int lineIndex, int startCol, int endCol,
                           bool isBold, bool isItalic) const {
    Line* line = editor.getLine(lineIndex);
    if (!line || !isValidColumnRange(editor, lineIndex, startCol, endCol) ||
        endCol == startCol || (!isBold && !isItalic)) return false;

    FormatSpan span;
    span.startCol = startCol;
    span.endCol = endCol;
    span.type = isBold && isItalic ? BOLD_ITALIC : (isBold ? BOLD : ITALIC);
    return line->formats.pushBack(span);
}

bool Formatting::removeFormat(Editor& editor, int lineIndex, int formatIndex) const {
    Line* line = editor.getLine(lineIndex);
    if (!line) return false;
    FormatSpan removed;
    return line->formats.removeAt(formatIndex, removed);
}

int Formatting::count(const Editor& editor, int lineIndex) const {
    if (lineIndex < 0 || lineIndex >= editor.getLineCount()) return 0;
    Line* line = const_cast<Editor&>(editor).getLine(lineIndex);
    return line ? line->formats.size() : 0;
}

std::string Formatting::applyFormatting(const Line& line) const {
    std::string result;
    const int len = static_cast<int>(line.text.size());
    for (int i = 0; i < len; ++i) {
        bool bold = false;
        bool italic = false;
        for (int j = 0; j < line.formats.size(); ++j) {
            const FormatSpan span = line.formats[j];
            if (i >= span.startCol && i < span.endCol) {
                bold = bold || span.type == BOLD || span.type == BOLD_ITALIC;
                italic = italic || span.type == ITALIC || span.type == BOLD_ITALIC;
            }
        }
        if (bold) result += "\033[1m";
        if (italic) result += "\033[3m";
        result += line.text[i];
        if (bold || italic) result += "\033[0m";
    }
    return result;
}

bool Formatting::onInsertText(Editor& editor, int lineIndex, int col, int length) const {
    Line* line = editor.getLine(lineIndex);
    if (!line || !isValidColumn(editor, lineIndex, col) || length < 0) return false;
    if (length == 0) return true;
    for (int i = 0; i < line->formats.size(); ++i) {
        FormatSpan span = line->formats[i];
        if (col <= span.startCol) {
            span.startCol += length;
            span.endCol += length;
        } else if (col < span.endCol) {
            span.endCol += length;
        }
        line->formats.set(i, span);
    }
    return true;
}

bool Formatting::onDeleteText(Editor& editor, int lineIndex, int col, int length) const {
    Line* line = editor.getLine(lineIndex);
    if (!line || !isValidColumn(editor, lineIndex, col) || length < 0) return false;
    if (length == 0) return true;
    const int end = col + length;
    for (int i = line->formats.size() - 1; i >= 0; --i) {
        FormatSpan span = line->formats[i];
        const auto shift = [col, end, length](int p) {
            if (p <= col) return p;
            if (p >= end) return p - length;
            return col;
        };
        span.startCol = shift(span.startCol);
        span.endCol = shift(span.endCol);
        if (span.startCol >= span.endCol) {
            FormatSpan removed;
            line->formats.removeAt(i, removed);
        } else {
            line->formats.set(i, span);
        }
    }
    return true;
}

bool Formatting::onSplitLine(Editor& editor, int lineIndex, int splitCol) const {
    Line* first = editor.getLine(lineIndex);
    Line* second = editor.getLine(lineIndex + 1);
    if (!first || !second || !isValidColumn(editor, lineIndex, splitCol)) return false;

    DynamicArray<FormatSpan> firstSpans;
    DynamicArray<FormatSpan> secondSpans;
    for (int i = 0; i < first->formats.size(); ++i) {
        const FormatSpan span = first->formats[i];
        if (span.startCol < splitCol) {
            FormatSpan left = span;
            left.endCol = span.endCol < splitCol ? span.endCol : splitCol;
            if (left.startCol < left.endCol) firstSpans.pushBack(left);
        }
        if (span.endCol > splitCol) {
            FormatSpan right = span;
            right.startCol = span.startCol > splitCol ? span.startCol - splitCol : 0;
            right.endCol = span.endCol - splitCol;
            if (right.startCol < right.endCol) secondSpans.pushBack(right);
        }
    }
    first->formats = firstSpans;
    for (int i = 0; i < second->formats.size(); ++i) secondSpans.pushBack(second->formats[i]);
    second->formats = secondSpans;
    return true;
}
