#include "editor/Display.h"
#include "editor/Formatting.h"

typedef DoublyLinkedList<Line>::Node Node;

// True for space, tab, newline, carriage return, vertical tab and form feed.
static bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

// Number of digits in n (n >= 0).
static int digitCount(int n) {
    int d = 1;
    while (n >= 10) { n /= 10; ++d; }
    return d;
}

// Print "  7 | " with the number right-aligned in `width` columns.
static void printLabel(std::ostream& out, int number, int width) {
    for (int i = digitCount(number); i < width; ++i) out << ' ';
    out << number << " | ";
}

// Node at index, walking from the nearer end. Returns nullptr if out of range.
static Node* nodeAtIndex(const DoublyLinkedList<Line>& doc, int index) {
    int n = doc.size();
    if (index < 0 || index >= n) return nullptr;

    Node* cur;
    if (index < n / 2) {
        cur = doc.getHead();
        for (int i = 0; i < index; ++i) cur = cur->next;
    } else {
        cur = doc.getTail();
        for (int i = n - 1; i > index; --i) cur = cur->prev;
    }
    return cur;
}

DocStats getStats(const Editor& ed) {
    DocStats s;
    s.lines = ed.document.size();
    s.words = 0;
    s.chars = 0;
    s.charsNoSpaces = 0;

    for (Node* cur = ed.document.getHead(); cur != nullptr; cur = cur->next) {
        const std::string& text = cur->data.text;
        s.chars += static_cast<int>(text.size());

        bool inWord = false;
        for (std::size_t i = 0; i < text.size(); ++i) {
            if (isWhitespace(text[i])) {
                inWord = false;
            } else {
                ++s.charsNoSpaces;
                if (!inWord) { ++s.words; inWord = true; }
            }
        }
    }
    return s;
}

void displayDocument(const Editor& ed, std::ostream& out) {
    if (ed.document.isEmpty()) {
        out << "(document is empty)\n";
        return;
    }

    int width = digitCount(ed.document.size());
    int number = 1;
    for (Node* cur = ed.document.getHead(); cur != nullptr; cur = cur->next, ++number) {
        printLabel(out, number, width);
        out << Formatting::applySpans(cur->data.text, cur->data.formats) << '\n';
    }
}

bool displayLine(const Editor& ed, int index, std::ostream& out) {
    if (ed.document.isEmpty()) {
        out << "Error: the document is empty.\n";
        return false;
    }

    Node* node = nodeAtIndex(ed.document, index);
    if (node == nullptr) {
        out << "Error: line " << (index + 1) << " does not exist (document has "
            << ed.document.size() << " lines).\n";
        return false;
    }

    printLabel(out, index + 1, digitCount(ed.document.size()));
    out << Formatting::applySpans(node->data.text, node->data.formats) << '\n';
    return true;
}

void displayStats(const Editor& ed, std::ostream& out) {
    DocStats s = getStats(ed);
    std::string filename = ed.getFilename();

    out << "--- Document info ---\n";
    out << "File       : " << (filename.empty() ? "(unsaved)" : filename) << '\n';
    out << "Lines      : " << s.lines << '\n';
    out << "Words      : " << s.words << '\n';
    out << "Characters : " << s.chars << " (" << s.charsNoSpaces << " without spaces)\n";
    out << "Modified   : " << (ed.isModified() ? "yes" : "no") << '\n';
}