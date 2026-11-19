#ifndef EDITOR_VALIDATION_H
#define EDITOR_VALIDATION_H

#include <string>
#include "ds/DoublyLinkedList.h"
#include "editor/Editor.h"

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

// Equivalent checks for the editor's Line-based document.
bool isDocumentEmpty(const Editor& editor);
bool isValidLine(const Editor& editor, int line);
bool isValidColumn(const Editor& editor, int line, int col);
bool isValidRange(const Editor& editor, int startLine, int endLine);
bool isValidColumnRange(const Editor& editor, int line, int startCol, int endCol);

// Replacement text must remain within one logical line.
bool isValidReplacementText(const std::string& text);

#endif // EDITOR_VALIDATION_H
