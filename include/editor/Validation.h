#ifndef EDITOR_VALIDATION_H
#define EDITOR_VALIDATION_H

#include <string>
#include "ds/DoublyLinkedList.h"

// Pure checks. They never print and never change the document, so every
// module can call them before touching the text. The caller decides what
// message to show. All line and column numbers are 0-based.

// True if the document has no lines.
bool isDocumentEmpty(const DoublyLinkedList<std::string>& doc);

// True if s has length 0. (A string of spaces is NOT empty: searching for
// " " is a valid request.)
bool isEmptyString(const std::string& s);

// True if 0 <= line < number of lines.
bool isValidLine(const DoublyLinkedList<std::string>& doc, int line);

// True if line exists and 0 <= col <= length of that line.
// col == length is allowed because it is a valid insertion point (end of line).
bool isValidColumn(const DoublyLinkedList<std::string>& doc, int line, int col);

// True if both lines exist and startLine <= endLine (inclusive range).
bool isValidRange(const DoublyLinkedList<std::string>& doc, int startLine, int endLine);

// True if line exists and 0 <= startCol <= endCol <= length of that line.
// endCol is exclusive, so startCol == endCol is an empty range.
bool isValidColumnRange(const DoublyLinkedList<std::string>& doc, int line,
                        int startCol, int endCol);

#endif // EDITOR_VALIDATION_H