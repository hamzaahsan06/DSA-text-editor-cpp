#ifndef EDITOR_VALIDATION_H
#define EDITOR_VALIDATION_H

#include <string>
#include "editor/Editor.h"

// True if the document has no lines.
bool isDocumentEmpty(const Editor& ed);

// True if s has length 0. (A string of spaces is NOT empty: searching for
// " " is a valid request.)
bool isEmptyString(const std::string& s);

// True if 0 <= line < number of lines.
bool isValidLine(const Editor& ed, int line);

// True if line exists and 0 <= col <= length of that line.
// col == length is allowed because it is a valid insertion point (end of line).
bool isValidColumn(const Editor& ed, int line, int col);

// True if both lines exist and startLine <= endLine (inclusive range).
bool isValidRange(const Editor& ed, int startLine, int endLine);

// True if line exists and 0 <= startCol <= endCol <= length of that line.
// endCol is exclusive, so startCol == endCol is an empty range.
bool isValidColumnRange(const Editor& ed, int line, int startCol, int endCol);

#endif // EDITOR_VALIDATION_H