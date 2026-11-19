#pragma once

#include <string>
#include "ds/DynamicArray.h"

// Represents a cursor or match position in the document (0-based).
struct Position
{
    int line = 0;
    int col = 0;
};

// The types of formatting a span can have.
enum FormatType
{
    BOLD,
    ITALIC,
    BOLD_ITALIC
};

// Represents a formatting block within a single line.
// The span covers columns [startCol, endCol) of the line's text.
struct FormatSpan
{
    int startCol = 0;
    int endCol = 0;
    FormatType type = BOLD;
};

// Represents a single line of text with its associated formatting.
struct Line
{
    std::string text = "";
    DynamicArray<FormatSpan> formats; // Formatting spans applied to this line.
};

// Represents a saved location in the document.
struct Bookmark
{
    std::string name = ""; // User-supplied label for the bookmark.
    int line = 0;
    int col = 0;
};

// The types of operations that can be undone/redone.
enum OpType
{
    INSERT_TEXT,
    DELETE_TEXT,
    INSERT_LINE,
    DELETE_LINE,
    REPLACE,
    FORMAT_APPLY,
    FORMAT_REMOVE
};

// Represents a single undoable action in the editor.
// The fields used depend on type: text-based operations use
// line/col/oldText/newText, while formatting operations use
// line/fmtStart/fmtEnd/fmtType. Both stacks (undo and redo)
// share this same record type.
struct Command
{
    OpType type = INSERT_TEXT;
    int line = 0;
    int col = 0;
    std::string oldText = ""; // Text before the operation (used to undo).
    std::string newText = ""; // Text after the operation (used to redo).
    int fmtStart = 0;
    int fmtEnd = 0;
    FormatType fmtType = BOLD;
    // REPLACE commands with this flag store complete line snapshots in oldText/newText.
    bool wholeLine = false;
};
