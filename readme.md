To run the program

Step 1 Make the build folder (once)
command:  
mkdir build -Force | Out-Null

Step 2 Build the editor

Command: 
g++ -std=c++11 -Wall -Wextra -Iinclude -Iinclude/editor src\main.cpp src\editor\EditorApp.cpp src\editor\CommandParser.cpp src\editor\Screen.cpp src\editor\Display.cpp src\editor\FileManager.cpp src\editor\TextOps.cpp src\editor\SearchReplace.cpp src\editor\Bookmarks.cpp src\editor\UndoRedo.cpp src\editor\Formatting.cpp src\editor\Validation.cpp -o build\editor.exe

step 3: Run the editor

Command:
.\build\editor.exe