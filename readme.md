# DSA Text Editor (C++ CLI)

A modular command-line text editor built in C++17 using custom-implemented data structures—without relying on STL containers (`std::vector`, `std::list`, `std::stack`) for core storage.

---

## Features

- **Document Buffer & Line Editing:** Insert text at the cursor or any `(line, column)`, split lines, and delete individual characters, words, column ranges, or multi-line blocks.
- **Multi-Level Undo / Redo:** Full command history supporting text insertion, deletion, line splits, multi-line deletions, and search-and-replace operations while keeping formatting spans and bookmarks synchronized.
- **Search & Replace:** Substring search across all lines with 1-based `(Line, Column)` match reporting, $N$-th occurrence replacement, and shift-safe `replaceall`.
- **ANSI Inline Formatting:** Apply **bold**, *italic*, or ***bold-italic*** styling to half-open column spans `[startCol, endCol)`. Spans automatically shift on insertion/deletion, split across new lines, and merge back on undo.
- **Dynamic Bookmarks:** Save named bookmarks that move automatically when lines are inserted or deleted above them, plus a built-in previous-position slot (`goto` / `back`).
- **File I/O & Document Stats:** Create, open (with automatic `\r\n` normalization), save, and inspect live line, word, and character counts (with and without spaces).

---

## Data Structures & Complexity

All underlying containers are implemented from scratch in `include/ds/` with deep-copy constructors, copy-assignment operators, and destructors:

| Structure | Header | Role in Editor | Time Complexity |
| :--- | :--- | :--- | :--- |
| **Doubly Linked List** | `include/ds/DoublyLinkedList.h` | Stores `Line` nodes (`document`), traversing from the nearer end (`head` or `tail`) | Head/Tail Insert: $O(1)$<br>Index Lookup: $O(n/2)$<br>Full Traversal: $O(n)$ |
| **Linked Stack (LIFO)** | `include/ds/Stack.h` | Manages `Command` records for `undoStack` and `redoStack` | Push / Pop / Peek: $O(1)$ |
| **Dynamic Array** | `include/ds/DynamicArray.h` | Resizable backing buffer for line `FormatSpan`s, `Bookmark`s, and search `Position`s | Append (`pushBack`): $O(1)$ amortized<br>Indexed Access: $O(1)$<br>Insert / Remove at index: $O(k)$ |

---

## Project Structure

```text
DSA-text-editor-cpp/
├── include/
│   ├── ds/
│   │   ├── DoublyLinkedList.h   # Generic doubly linked list for document lines
│   │   ├── DynamicArray.h       # Resizable array for spans, bookmarks, and matches
│   │   ├── SinglyLinkedList.h   # Generic singly linked list
│   │   └── Stack.h              # Generic linked stack for undo/redo history
│   └── editor/
│       ├── Bookmarks.h          # Named bookmark management & line-shift tracking
│       ├── Display.h            # Line-numbered rendering & document statistics
│       ├── Editor.h             # Core editor state and document controller
│       ├── FileManager.h        # Disk load/save operations & unsaved-state checks
│       ├── Formatting.h         # ANSI escape formatting for [startCol, endCol) spans
│       ├── SearchReplace.h      # Substring search, Nth replace, and replace-all
│       ├── TextOps.h            # Text/line insertion, deletion, and span updates
│       ├── Types.h              # Position, FormatSpan, Line, Bookmark, Command structs
│       ├── UndoRedo.h           # Undo/redo command execution and history management
│       └── Validation.h         # Line, column, and range bounds validation
├── src/
│   ├── editor/
│   │   ├── Bookmarks.cpp
│   │   ├── Display.cpp
│   │   ├── FileManager.cpp
│   │   ├── Formatting.cpp
│   │   ├── SearchReplace.cpp
│   │   ├── TextOps.cpp
│   │   ├── UndoRedo.cpp
│   │   └── Validation.cpp
│   └── main.cpp                 # Interactive CLI loop and command parser
├── tests/
│   ├── ds/                      # Unit tests for data structures
│   └── editor/                  # Unit tests for editor modules
├── LICENSE
└── readme.md