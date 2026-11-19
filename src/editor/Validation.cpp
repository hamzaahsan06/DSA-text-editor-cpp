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

bool isDocumentEmpty(const Editor& editor) {
    return editor.getLineCount() == 0;
}

bool isValidLine(const Editor& editor, int line) {
    return line >= 0 && line < editor.getLineCount();
}

bool isValidColumn(const Editor& editor, int line, int col) {
    const Line* target = editor.getLine(line);
    return target && col >= 0 && col <= static_cast<int>(target->text.size());
}

bool isValidRange(const Editor& editor, int startLine, int endLine) {
    return isValidLine(editor, startLine) && isValidLine(editor, endLine)
        && startLine <= endLine;
}

bool isValidColumnRange(const Editor& editor, int line, int startCol, int endCol) {
    const Line* target = editor.getLine(line);
    return target && startCol >= 0 && startCol <= endCol
        && endCol <= static_cast<int>(target->text.size());
}

bool isValidReplacementText(const std::string& text) {
    return text.find_first_of("\r\n") == std::string::npos;
}
