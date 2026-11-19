// include/editor/FileManager.h
#ifndef EDITOR_FILE_MANAGER_H
#define EDITOR_FILE_MANAGER_H

#include <string>
#include "editor/Editor.h"
#include "editor/Types.h"

// Resets the editor to a blank, untitled document.
bool newDocument(Editor &ed);

// Opens path. On failure, the current editor state is preserved.
bool openFile(Editor &ed, const std::string &path);

// Saves to the current filename, or returns false when it has none.
bool saveFile(Editor &ed);

// Saves to path and updates the editor filename only after a successful write.
bool saveFileAs(Editor &ed, const std::string &path);

// Returns true when the editor has no unsaved changes.
bool exitEditor(Editor &ed);

#endif
