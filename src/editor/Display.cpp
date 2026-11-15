#include "editor/Display.h"

typedef DoublyLinkedList<std::string>::Node Node;

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

DocStats getStats(const DoublyLinkedList<std::string>& doc) {
    DocStats s;
    s.lines = doc.size();
    s.words = 0;
    s.chars = 0;
    s.charsNoSpaces = 0;

    for (Node* cur = doc.getHead(); cur != nullptr; cur = cur->next) {
        const std::string& text = cur->data;
        s.chars += static_cast<int>(text.size());

        bool inWord = false;                 // words never span two lines
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

void displayDocument(const DoublyLinkedList<std::string>& doc, std::ostream& out) {
    if (doc.isEmpty()) {
        out << "(document is empty)\n";
        return;
    }

    int width = digitCount(doc.size());
    int number = 1;
    // Follow node pointers: O(n). Calling get(i) in a loop would be O(n^2).
    for (Node* cur = doc.getHead(); cur != nullptr; cur = cur->next, ++number) {
        printLabel(out, number, width);
        out << cur->data << '\n';
    }
}

bool displayLine(const DoublyLinkedList<std::string>& doc, int index, std::ostream& out) {
    if (doc.isEmpty()) {
        out << "Error: the document is empty.\n";
        return false;
    }

    std::string text;
    if (!doc.get(index, text)) {
        out << "Error: line " << (index + 1) << " does not exist (document has "
            << doc.size() << " lines).\n";
        return false;
    }

    printLabel(out, index + 1, digitCount(doc.size()));
    out << text << '\n';
    return true;
}

void displayStats(const DoublyLinkedList<std::string>& doc,
                  const std::string& filename, bool modified, std::ostream& out) {
    DocStats s = getStats(doc);

    out << "--- Document info ---\n";
    out << "File       : " << (filename.empty() ? "(unsaved)" : filename) << '\n';
    out << "Lines      : " << s.lines << '\n';
    out << "Words      : " << s.words << '\n';
    out << "Characters : " << s.chars << " (" << s.charsNoSpaces << " without spaces)\n";
    out << "Modified   : " << (modified ? "yes" : "no") << '\n';
}