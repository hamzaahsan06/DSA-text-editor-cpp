#ifndef EDITOR_DISPLAY_H
#define EDITOR_DISPLAY_H

#include <iostream>
#include <string>
#include "ds/DoublyLinkedList.h"

// Counts for the whole document. Newlines are not counted as characters.
struct DocStats {
    int lines;
    int words;
    int chars;          // all characters, spaces included
    int charsNoSpaces;  // characters that are not whitespace
};

// Compute the counts in one pass. Empty document gives all zeros.
DocStats getStats(const DoublyLinkedList<std::string>& doc);

// Print the whole document with 1-based line numbers.
// An empty document prints "(document is empty)".
void displayDocument(const DoublyLinkedList<std::string>& doc,
                     std::ostream& out = std::cout);

// Print one line. index is 0-based (same as the list); the label shown is
// 1-based. Returns false and prints an error if the index is invalid
// or the document is empty.
bool displayLine(const DoublyLinkedList<std::string>& doc, int index,
                 std::ostream& out = std::cout);

// Print file name, line / word / character counts and the unsaved flag.
void displayStats(const DoublyLinkedList<std::string>& doc,
                  const std::string& filename, bool modified,
                  std::ostream& out = std::cout);

#endif // EDITOR_DISPLAY_H