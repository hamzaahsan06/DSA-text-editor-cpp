// g++ -std=c++11 -Wall -Wextra -Iinclude -Iinclude/editor src/main.cpp src/editor/EditorApp.cpp src/editor/CommandParser.cpp src/editor/Screen.cpp src/editor/Display.cpp src/editor/FileManager.cpp src/editor/TextOps.cpp src/editor/SearchReplace.cpp src/editor/Bookmarks.cpp src/editor/UndoRedo.cpp src/editor/Formatting.cpp src/editor/Validation.cpp -o build/TextEditor.exe ; if ($?) { ./build/TextEditor.exe }

#include "editor/EditorApp.h"

// Entry point: create the app, run its main loop, exit cleanly.
int main()
{
    EditorApp app;
    app.run();
    return 0;
}