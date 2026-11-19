#pragma once

#include <string>
#include <iostream>
#include "editor/Editor.h"
#include "editor/Types.h"
#include "editor/UndoRedo.h"
#include "ds/DynamicArray.h"

// Returns index of first match of pattern in text at/after startPos, or -1.
int findSubstring(const std::string& text, const std::string& pattern, int startPos);

// Returns every match position (line, col) in the document.
DynamicArray<Position> searchAll(Editor& ed, const std::string& pattern);

// Returns true if pattern occurs anywhere in the document.
bool containsText(Editor& ed, const std::string& pattern);

// Prints search results: "Not found" or "Found N occurrence(s):" then positions (1-based).
void displaySearchResults(const DynamicArray<Position>& results, std::ostream& out);
void displaySearchResults(const DynamicArray<Position>& results,
                          const std::string& pattern, std::ostream& out);

// Replaces the nth match (n starts at 1). Returns false on bad input or too few matches.
bool replaceOccurrence(Editor& ed, const std::string& pattern, const std::string& newStr, int n);

// Replaces every match left to right; returns count. Safe against newStr containing pattern.
int replaceAll(Editor& ed, const std::string& pattern, const std::string& newStr);
