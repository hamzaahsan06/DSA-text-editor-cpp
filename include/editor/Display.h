#ifndef EDITOR_DISPLAY_H
#define EDITOR_DISPLAY_H

#include <iostream>
#include <string>
#include "editor/Editor.h"

// Counts for the whole document. Newlines are not counted as characters.
struct DocStats {
    int lines;
    int words;
    int chars;          // all characters, spaces included
    int charsNoSpaces;  // characters that are not whitespace
};

// Compute the counts in one pass. A document with no lines gives all zeros.
DocStats getStats(const Editor& ed);

// Print the whole document with 1-based line numbers.
// A document with no lines prints "(document is empty)".
void displayDocument(const Editor& ed, std::ostream& out = std::cout);

// Print one line. index is 0-based (same as the list); the label shown is
// 1-based. Returns false and prints an error if the index is invalid
// or the document has no lines.
bool displayLine(const Editor& ed, int index, std::ostream& out = std::cout);

// Print file name, line / word / character counts and the unsaved flag.
void displayStats(const Editor& ed, std::ostream& out = std::cout);

#endif // EDITOR_DISPLAY_H