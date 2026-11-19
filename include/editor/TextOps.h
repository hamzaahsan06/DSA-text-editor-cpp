#ifndef EDITOR_TEXT_OPS_H
#define EDITOR_TEXT_OPS_H

#include <string>
#include "editor/Editor.h"

// Text insertion and deletion.
//
// Conventions (same as Validation, Display and FileManager):
//   - line and col are 0-based. The menu converts the user's 1-based numbers.
//   - Every function returns true on success. On failure it prints one error
//     line, changes nothing and returns false.
//   - On success it sets the modified flag, moves the cursor and records the
//     edit for undo.

// Insert text into one line at (line, col). col == line length appends.
// text must not be empty and must not contain '\n' (use insertNewLine).
// If the document has no lines, (0, 0) first creates one empty line.
// Format spans at or after col shift right; a span grows only when the text
// goes strictly inside it (inserting exactly at its end does not extend it).
// Cursor ends just after the inserted text.
bool insertText(Editor& ed, int line, int col, const std::string& text);

// Split a line at (line, col): the text from col onward moves to a new line
// below. col == line length adds an empty line below; col == 0 pushes the whole
// line down. If the document has no lines, (0, 0) first creates one empty line.
// Format spans are divided between the two lines (a span crossing col is cut).
// Cursor ends at the start of the new line.
bool insertNewLine(Editor& ed, int line, int col);

// Delete the characters [startCol, endCol) from one line (endCol exclusive).
// An empty range (startCol == endCol) is rejected.
// Cursor ends at (line, startCol).
bool deleteText(Editor& ed, int line, int startCol, int endCol);

// Delete the single character at (line, col). col must be < line length.
// Cursor ends at (line, col).
bool deleteChar(Editor& ed, int line, int col);

// Delete the word that contains (line, col). A word is a run of
// non-whitespace characters. Rejected if the character at col is whitespace
// or col is at/after the end of the line. Surrounding spaces are kept.
// Cursor ends at the position where the word started.
bool deleteWord(Editor& ed, int line, int col);

// Delete lines startLine..endLine (both inclusive). Use the same value twice
// to delete one line. Deleting every line leaves a document with zero lines.
// Cursor ends on startLine, or on the last line if startLine no longer exists.
bool deleteLines(Editor& ed, int startLine, int endLine);

#endif // EDITOR_TEXT_OPS_H