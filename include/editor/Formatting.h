#ifndef FORMATTING_H
#define FORMATTING_H

#include <string>
#include "editor/Types.h"

// Adds a format span [startCol, endCol) to the line's formats. O(n).
bool addFormat(Line &line, int startCol, int endCol, bool isBold, bool isItalic);

// Removes the format span at the given 0-based index. O(n).
bool removeFormat(Line &line, int spanIndex);

// Removes all format spans from the line. O(1).
void clearFormats(Line &line);

// Renders the line, applying ANSI codes or markdown-style fallback. O(n*m).
std::string renderLine(const Line &line, bool useAnsi);

#endif