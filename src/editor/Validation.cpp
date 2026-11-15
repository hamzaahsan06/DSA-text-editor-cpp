#include "editor/Validation.h"

bool isDocumentEmpty(const DoublyLinkedList<std::string>& doc) {
    return doc.isEmpty();
}

bool isEmptyString(const std::string& s) {
    return s.empty();
}

bool isValidLine(const DoublyLinkedList<std::string>& doc, int line) {
    return line >= 0 && line < doc.size();
}

bool isValidColumn(const DoublyLinkedList<std::string>& doc, int line, int col) {
    std::string text;
    if (!doc.get(line, text)) return false;      // also rejects a bad line
    return col >= 0 && col <= static_cast<int>(text.size());
}

bool isValidRange(const DoublyLinkedList<std::string>& doc, int startLine, int endLine) {
    return isValidLine(doc, startLine) && isValidLine(doc, endLine)
        && startLine <= endLine;
}

bool isValidColumnRange(const DoublyLinkedList<std::string>& doc, int line,
                        int startCol, int endCol) {
    std::string text;
    if (!doc.get(line, text)) return false;
    return startCol >= 0 && startCol <= endCol
        && endCol <= static_cast<int>(text.size());
}