# Text Editor in C++

A command-driven text editor developed in C++11 as a Data Structures and Algorithms (DSA) semester project. The project implements core data structures from scratch to support text editing, file management, searching, formatting, and undo/redo operations.

## Features

- **File Management:** Create, open, save, and save files under new names.
- **Text Editing:** Insert text, split lines, and delete characters, words, ranges, and lines.
- **Cursor Navigation:** Move to specific lines and columns.
- **Search & Replace:** Find text, replace specific occurrences, or replace all.
- **Undo & Redo:** Revert and reapply editing operations.
- **Bookmarks:** Save named positions and navigate between them.
- **Text Formatting:** Apply bold and italic formatting.
- **Document Statistics:** View line, word, and character counts.
- **Console Interface:** Command-based interface with a live screen and status bar.

## Data Structures Implemented

- **Doubly Linked List** – Stores document lines.
- **Singly Linked List** – Generic linked-list ADT.
- **Dynamic Array** – Stores resizable collections.
- **Stack** – Manages undo and redo history.

## Project Structure

```text
├── include/
│   ├── ds/         # Data structure implementations
│   └── editor/     # Editor module headers
├── src/
│   ├── main.cpp
│   └── editor/     # Editor module implementations
├── tests/
│   ├── ds/         # Data structure tests
│   └── editor/     # Editor module and integration tests
├── LICENSE
└── readme.md
```

## Requirements

- C++ compiler supporting C++11 or later
- GCC (MinGW recommended on Windows)

## Build & Run (Windows / PowerShell)

**1. Create the build directory**

```powershell
mkdir build -Force | Out-Null
```

**2. Compile the project**

```powershell
g++ -std=c++11 -Wall -Wextra -Iinclude -Iinclude/editor src\main.cpp src\editor\EditorApp.cpp src\editor\CommandParser.cpp src\editor\Screen.cpp src\editor\Display.cpp src\editor\FileManager.cpp src\editor\TextOps.cpp src\editor\SearchReplace.cpp src\editor\Bookmarks.cpp src\editor\UndoRedo.cpp src\editor\Formatting.cpp src\editor\Validation.cpp -o build\editor.exe
```

**3. Run the editor**

```powershell
.\build\editor.exe
```

## Usage

Enter `help` inside the editor to view available commands. Use `help <command>` to view details and usage examples for a specific command.

## License

This project is licensed under the [MIT License](LICENSE).
