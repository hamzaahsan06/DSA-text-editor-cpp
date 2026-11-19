#include "editor/Validation.h"

typedef DoublyLinkedList<Line>::Node Node;

// Length of the text on `line`, or -1 if the line does not exist.
// Walks from the nearer end and reads the node in place, so the Line
// (and its formats array) is never copied.
static int lineLength(const Editor& ed, int line) {
    int n = ed.document.size();
    if (line < 0 || line >= n) return -1;

    Node* cur;
    if (line < n / 2) {
        cur = ed.document.getHead();
        for (int i = 0; i < line; ++i) cur = cur->next;
    } else {
        cur = ed.document.getTail();
        for (int i = n - 1; i > line; --i) cur = cur->prev;
    }
    return static_cast<int>(cur->data.text.size());
}

bool isDocumentEmpty(const Editor& ed) {
    return ed.document.isEmpty();
}

bool isEmptyString(const std::string& s) {
    return s.empty();
}

bool isValidLine(const Editor& ed, int line) {
    return line >= 0 && line < ed.document.size();
}

bool isValidColumn(const Editor& ed, int line, int col) {
    int len = lineLength(ed, line);
    if (len < 0) return false;                   // also rejects a bad line
    return col >= 0 && col <= len;
}

bool isValidRange(const Editor& ed, int startLine, int endLine) {
    return isValidLine(ed, startLine) && isValidLine(ed, endLine)
        && startLine <= endLine;
}

bool isValidColumnRange(const Editor& ed, int line, int startCol, int endCol) {
    int len = lineLength(ed, line);
    if (len < 0) return false;
    return startCol >= 0 && startCol <= endCol && endCol <= len;
}