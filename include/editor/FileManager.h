// include/editor/FileManager.h
#ifndef EDITOR_FILE_MANAGER_H
#define EDITOR_FILE_MANAGER_H

#include <string>
#include "editor/Editor.h"
#include "editor/Types.h"

bool newDocument(Editor& ed);
bool openFile(Editor& ed, const std::string& path);
bool saveFile(Editor& ed);
bool saveFileAs(Editor& ed, const std::string& path);
bool exitEditor(Editor& ed);

#endif