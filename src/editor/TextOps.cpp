#include "editor/TextOps.h"

#include <iostream>
#include "editor/Validation.h"
#include "editor/UndoRedo.h"
#include "editor/Bookmarks.h"

// True for space, tab, newline, carriage return, vertical tab and form feed.
static bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

// Prints an error and returns false if the document is empty or line does not exist.
static bool requireLine(const Editor& ed, int line) {
    if (isDocumentEmpty(ed)) {
        std::cout << "Error: the document is empty.\n";
        return false;
    }
    if (!isValidLine(ed, line)) {
        std::cout << "Error: line " << (line + 1) << " does not exist (document has "
                  << ed.document.size() << " lines).\n";
        return false;
    }
    return true;
}

// Prints an error and returns false if col is not 0..length of the line.
static bool requireColumn(const Editor& ed, int line, int col) {
    if (!isValidColumn(ed, line, col)) {
        std::cout << "Error: column " << (col + 1) << " is outside line " << (line + 1) << ".\n";
        return false;
    }
    return true;
}

// For the two insert functions. A document with no lines accepts only (0, 0):
// one empty line is created first so there is something to insert into.
// Returns false (and changes nothing) for any other position.
static bool openEmptyDocument(Editor& ed, int line, int col) {
    if (!isDocumentEmpty(ed)) return true;
    if (line != 0 || col != 0) {
        std::cout << "Error: the document is empty; insert at line 1, column 1.\n";
        return false;
    }
    Line first;
    ed.document.insertAtTail(first);
    return true;
}

static void moveCursor(Editor& ed, int line, int col) {
    ed.cursorLine = line;
    ed.cursorCol = col;
}

// NOTE: formatting spans (Line::formats) are not shifted here. Formatting is
// not connected to Line::formats yet; add the shifting where marked once it is.

bool insertText(Editor& ed, int line, int col, const std::string& text) {
    if (isEmptyString(text)) {
        std::cout << "Error: there is no text to insert.\n";
        return false;
    }
    if (text.find('\n') != std::string::npos) {
        std::cout << "Error: text cannot contain a newline; use insert new line.\n";
        return false;
    }
    if (!openEmptyDocument(ed, line, col)) return false;
    if (!requireLine(ed, line) || !requireColumn(ed, line, col)) return false;

    Line* target = ed.getLine(line);
    target->text.insert(col, text);
    // (shift formatting spans on this line here)

    UndoRedo history;
    history.recordInsert(ed, line, col, text);

    ed.setModified(true);
    moveCursor(ed, line, col + static_cast<int>(text.size()));
    return true;
}

bool insertNewLine(Editor& ed, int line, int col) {
    if (!openEmptyDocument(ed, line, col)) return false;
    if (!requireLine(ed, line) || !requireColumn(ed, line, col)) return false;

    // Left part stays on this line; the right part becomes a new line below.
    Line* target = ed.getLine(line);
    Line below;
    below.text = target->text.substr(col);
    target->text.erase(col);
    ed.document.insertAtPosition(line + 1, below);
    adjustBookmarksOnLineInsert(ed, line + 1);

    // Encoding: line = the line that was split, col = where it was split.
    Command op;
    op.type = INSERT_LINE;
    op.line = line;
    op.col = col;
    UndoRedo history;
    history.record(ed, op);

    ed.setModified(true);
    moveCursor(ed, line + 1, 0);
    return true;
}

bool deleteText(Editor& ed, int line, int startCol, int endCol) {
    if (!requireLine(ed, line)) return false;
    if (!isValidColumnRange(ed, line, startCol, endCol)) {
        std::cout << "Error: columns " << (startCol + 1) << " to " << endCol
                  << " are not inside line " << (line + 1) << ".\n";
        return false;
    }
    if (startCol == endCol) {
        std::cout << "Error: nothing to delete (empty range).\n";
        return false;
    }

    Line* target = ed.getLine(line);
    std::string removed = target->text.substr(startCol, endCol - startCol);
    target->text.erase(startCol, endCol - startCol);
    // (shift formatting spans on this line here)

    UndoRedo history;
    history.recordErase(ed, line, startCol, removed);

    ed.setModified(true);
    moveCursor(ed, line, startCol);
    return true;
}

bool deleteChar(Editor& ed, int line, int col) {
    if (!requireLine(ed, line)) return false;

    int length = static_cast<int>(ed.getLine(line)->text.size());
    if (col < 0 || col >= length) {
        std::cout << "Error: there is no character at column " << (col + 1)
                  << " of line " << (line + 1) << ".\n";
        return false;
    }
    return deleteText(ed, line, col, col + 1);
}

bool deleteWord(Editor& ed, int line, int col) {
    if (!requireLine(ed, line)) return false;

    const std::string& text = ed.getLine(line)->text;
    int length = static_cast<int>(text.size());
    if (col < 0 || col >= length || isWhitespace(text[col])) {
        std::cout << "Error: there is no word at line " << (line + 1)
                  << ", column " << (col + 1) << ".\n";
        return false;
    }

    int start = col;
    while (start > 0 && !isWhitespace(text[start - 1])) --start;
    int end = col;
    while (end < length && !isWhitespace(text[end])) ++end;

    return deleteText(ed, line, start, end);
}

bool deleteLines(Editor& ed, int startLine, int endLine) {
    if (isDocumentEmpty(ed)) {
        std::cout << "Error: the document is empty.\n";
        return false;
    }
    if (!isValidRange(ed, startLine, endLine)) {
        std::cout << "Error: lines " << (startLine + 1) << " to " << (endLine + 1)
                  << " are not a valid range (document has " << ed.document.size()
                  << " lines).\n";
        return false;
    }

    // The lines below slide up after each removal, so always remove at
    // startLine. Each removed line is kept for undo, joined with '\n'.
    int count = endLine - startLine + 1;
    std::string removedText;
    for (int i = 0; i < count; ++i) {
        Line removed;
        ed.document.removeAtPosition(startLine, removed);
        if (i > 0) removedText += '\n';
        removedText += removed.text;
        adjustBookmarksOnLineDelete(ed, startLine);
    }

    // Encoding: line = first deleted line, oldText = deleted lines joined by '\n'.
    Command op;
    op.type = DELETE_LINE;
    op.line = startLine;
    op.col = 0;
    op.oldText = removedText;
    UndoRedo history;
    history.record(ed, op);

    ed.setModified(true);
    int remaining = ed.document.size();
    if (remaining == 0)
        moveCursor(ed, 0, 0);
    else
        moveCursor(ed, startLine < remaining ? startLine : remaining - 1, 0);
    return true;
}