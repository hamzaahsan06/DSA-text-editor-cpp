#include <iostream>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "editor/Bookmarks.h"
#include "editor/Display.h"
#include "editor/Editor.h"
#include "editor/FileManager.h"
#include "editor/Formatting.h"
#include "editor/SearchReplace.h"
#include "editor/TextOps.h"
#include "editor/Types.h"
#include "editor/UndoRedo.h"
#include "editor/Validation.h"

// Enables ANSI escape codes (\033[1m, \033[3m) on Windows consoles.
static void enableAnsiColors()
{
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE)
    {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode))
        {
            SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
#endif
}

static std::string trim(const std::string &s)
{
    std::size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n'))
    {
        ++start;
    }
    std::size_t end = s.size();
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n'))
    {
        --end;
    }
    return s.substr(start, end - start);
}

static std::string readPrompt(const std::string &prompt)
{
    std::cout << prompt;
    std::string input;
    if (!std::getline(std::cin, input))
    {
        return "";
    }
    if (!input.empty() && input.back() == '\r')
    {
        input.pop_back();
    }
    return input;
}

static bool readIntPrompt(const std::string &prompt, int &out)
{
    std::string raw = trim(readPrompt(prompt));
    if (raw.empty())
    {
        return false;
    }
    std::istringstream iss(raw);
    int val = 0;
    char extra = '\0';
    if (!(iss >> val) || (iss >> extra))
    {
        std::cout << "Error: please enter a valid integer.\n";
        return false;
    }
    out = val;
    return true;
}

static void printHelp()
{
    std::cout << "\n=== Available Commands ===\n"
              << "File:\n"
              << "  new          - Start a new blank document\n"
              << "  open         - Open a text file from disk\n"
              << "  save         - Save current file\n"
              << "  saveas       - Save current document to a new path\n"
              << "  exit         - Exit the editor\n"
              << "\nView & Navigation:\n"
              << "  view         - Display the full document\n"
              << "  line         - Display a single line\n"
              << "  stats        - Show document statistics\n"
              << "  goto         - Move cursor to (line, column)\n"
              << "  back         - Return cursor to previous position\n"
              << "\nEditing (1-based line & column numbers):\n"
              << "  insert       - Insert text at current cursor\n"
              << "  insertat     - Insert text at a specific (line, column)\n"
              << "  newline      - Split line / insert newline at cursor\n"
              << "  delchar      - Delete character at cursor (or specified position)\n"
              << "  delword      - Delete word at cursor (or specified position)\n"
              << "  delrange     - Delete column range [start, end) on a line\n"
              << "  dellines     - Delete a range of lines\n"
              << "\nSearch, Replace & History:\n"
              << "  find         - Search for all occurrences of a pattern\n"
              << "  replace      - Replace the Nth occurrence of a pattern\n"
              << "  replaceall   - Replace all occurrences of a pattern\n"
              << "  undo         - Undo last edit\n"
              << "  redo         - Redo last undone edit\n"
              << "\nBookmarks & Formatting:\n"
              << "  bm_add       - Bookmark current cursor position\n"
              << "  bm_list      - List all bookmarks\n"
              << "  bm_goto      - Jump cursor to a named bookmark\n"
              << "  bm_del       - Delete a named bookmark\n"
              << "  format       - Apply bold/italic formatting to a column range\n"
              << "  clearfmt     - Clear formatting on a line\n"
              << "  help         - Show this command list\n\n";
}

static bool confirmDiscardIfModified(Editor &ed)
{
    if (!ed.isModified())
    {
        return true;
    }
    std::string ans = trim(readPrompt("Unsaved changes will be lost. Continue? (y/n): "));
    return (ans == "y" || ans == "Y" || ans == "yes" || ans == "YES");
}

int main(int argc, char *argv[])
{
    enableAnsiColors();

    Editor ed;
    UndoRedo undoRedo;

    std::cout << "DSA C++ CLI Text Editor (type 'help' for commands)\n";

    // Optional file path passed via command line: ./text_editor.exe notes.txt
    if (argc > 1)
    {
        if (openFile(ed, argv[1]))
        {
            std::cout << "Opened '" << argv[1] << "'.\n";
        }
    }

    while (true)
    {
        std::string name = ed.getFilename().empty() ? "untitled" : ed.getFilename();
        std::cout << "\n[" << name << (ed.isModified() ? "*" : "")
                  << " | Ln " << (ed.cursorLine + 1)
                  << ", Col " << (ed.cursorCol + 1) << "] > ";

        std::string cmdLine;
        if (!std::getline(std::cin, cmdLine))
        {
            break; // EOF
        }
        std::string cmd = trim(cmdLine);
        if (cmd.empty())
        {
            continue;
        }

        if (cmd == "help")
        {
            printHelp();
        }
        else if (cmd == "new")
        {
            if (confirmDiscardIfModified(ed))
            {
                newDocument(ed);
                std::cout << "Started a new document.\n";
            }
        }
        else if (cmd == "open")
        {
            if (confirmDiscardIfModified(ed))
            {
                std::string path = trim(readPrompt("File path to open: "));
                if (openFile(ed, path))
                {
                    std::cout << "Loaded '" << path << "'.\n";
                }
            }
        }
        else if (cmd == "save")
        {
            if (saveFile(ed))
            {
                std::cout << "Saved '" << ed.getFilename() << "'.\n";
            }
        }
        else if (cmd == "saveas")
        {
            std::string path = trim(readPrompt("Save as file path: "));
            if (saveFileAs(ed, path))
            {
                std::cout << "Saved as '" << path << "'.\n";
            }
        }
        else if (cmd == "exit" || cmd == "quit")
        {
            if (exitEditor(ed) || confirmDiscardIfModified(ed))
            {
                std::cout << "Exiting editor.\n";
                break;
            }
        }
        else if (cmd == "view")
        {
            displayDocument(ed, std::cout);
        }
        else if (cmd == "line")
        {
            int lineNum = 0;
            if (readIntPrompt("Line number (1-based): ", lineNum))
            {
                displayLine(ed, lineNum - 1, std::cout);
            }
        }
        else if (cmd == "stats")
        {
            displayStats(ed, std::cout);
        }
        else if (cmd == "goto")
        {
            int lineNum = 0, colNum = 0;
            if (readIntPrompt("Target line (1-based): ", lineNum) &&
                readIntPrompt("Target column (1-based): ", colNum))
            {
                int targetLine = lineNum - 1;
                int targetCol = colNum - 1;
                if (!isValidLine(ed, targetLine))
                {
                    std::cout << "Error: line " << lineNum << " does not exist.\n";
                }
                else if (!isValidColumn(ed, targetLine, targetCol))
                {
                    std::cout << "Error: column " << colNum << " is out of range on line " << lineNum << ".\n";
                }
                else
                {
                    savePreviousPosition(ed);
                    ed.cursorLine = targetLine;
                    ed.cursorCol = targetCol;
                    std::cout << "Cursor moved to Line " << lineNum << ", Column " << colNum << ".\n";
                }
            }
        }
        else if (cmd == "back")
        {
            if (returnToPreviousPosition(ed))
            {
                std::cout << "Returned to Line " << (ed.cursorLine + 1)
                          << ", Column " << (ed.cursorCol + 1) << ".\n";
            }
            else
            {
                std::cout << "No valid previous position saved.\n";
            }
        }
        else if (cmd == "insert")
        {
            std::string text = readPrompt("Text to insert at cursor: ");
            if (insertText(ed, ed.cursorLine, ed.cursorCol, text))
            {
                std::cout << "Inserted.\n";
            }
        }
        else if (cmd == "insertat")
        {
            int lineNum = 0, colNum = 0;
            if (readIntPrompt("Line (1-based): ", lineNum) &&
                readIntPrompt("Column (1-based): ", colNum))
            {
                std::string text = readPrompt("Text to insert: ");
                if (insertText(ed, lineNum - 1, colNum - 1, text))
                {
                    std::cout << "Inserted.\n";
                }
            }
        }
        else if (cmd == "newline")
        {
            if (insertNewLine(ed, ed.cursorLine, ed.cursorCol))
            {
                std::cout << "Inserted new line.\n";
            }
        }
        else if (cmd == "delchar")
        {
            if (deleteChar(ed, ed.cursorLine, ed.cursorCol))
            {
                std::cout << "Character deleted.\n";
            }
        }
        else if (cmd == "delword")
        {
            if (deleteWord(ed, ed.cursorLine, ed.cursorCol))
            {
                std::cout << "Word deleted.\n";
            }
        }
        else if (cmd == "delrange")
        {
            int lineNum = 0, startCol = 0, endCol = 0;
            if (readIntPrompt("Line (1-based): ", lineNum) &&
                readIntPrompt("Start column (1-based, inclusive): ", startCol) &&
                readIntPrompt("End column (1-based, exclusive): ", endCol))
            {
                if (deleteText(ed, lineNum - 1, startCol - 1, endCol - 1))
                {
                    std::cout << "Range deleted.\n";
                }
            }
        }
        else if (cmd == "dellines")
        {
            int startLine = 0, endLine = 0;
            if (readIntPrompt("Start line (1-based): ", startLine) &&
                readIntPrompt("End line (1-based, inclusive): ", endLine))
            {
                if (deleteLines(ed, startLine - 1, endLine - 1))
                {
                    std::cout << "Line(s) deleted.\n";
                }
            }
        }
        else if (cmd == "find")
        {
            std::string pattern = readPrompt("Search pattern: ");
            DynamicArray<Position> matches = searchAll(ed, pattern);
            displaySearchResults(matches, std::cout);
        }
        else if (cmd == "replace")
        {
            std::string pattern = readPrompt("Find pattern: ");
            std::string replacement = readPrompt("Replace with: ");
            int n = 0;
            if (readIntPrompt("Occurrence number (1-based): ", n))
            {
                if (replaceOccurrence(ed, pattern, replacement, n))
                {
                    std::cout << "Replaced occurrence #" << n << ".\n";
                }
                else
                {
                    std::cout << "Error: could not replace occurrence #" << n << ".\n";
                }
            }
        }
        else if (cmd == "replaceall")
        {
            std::string pattern = readPrompt("Find pattern: ");
            std::string replacement = readPrompt("Replace with: ");
            int count = replaceAll(ed, pattern, replacement);
            std::cout << "Replaced " << count << " occurrence(s).\n";
        }
        else if (cmd == "undo")
        {
            if (undoRedo.undo(ed))
            {
                std::cout << "Undone.\n";
            }
            else
            {
                std::cout << "Nothing to undo.\n";
            }
        }
        else if (cmd == "redo")
        {
            if (undoRedo.redo(ed))
            {
                std::cout << "Redone.\n";
            }
            else
            {
                std::cout << "Nothing to redo.\n";
            }
        }
        else if (cmd == "bm_add")
        {
            std::string bmName = trim(readPrompt("Bookmark name (no spaces): "));
            if (addBookmarkAtCursor(ed, bmName))
            {
                std::cout << "Bookmark '" << bmName << "' saved at Line "
                          << (ed.cursorLine + 1) << ", Column " << (ed.cursorCol + 1) << ".\n";
            }
            else
            {
                std::cout << "Error: invalid, duplicate, or reserved bookmark name.\n";
            }
        }
        else if (cmd == "bm_list")
        {
            listBookmarks(ed, std::cout);
        }
        else if (cmd == "bm_goto")
        {
            std::string bmName = trim(readPrompt("Bookmark name: "));
            savePreviousPosition(ed);
            if (gotoBookmark(ed, bmName))
            {
                std::cout << "Jumped to '" << bmName << "' (Line "
                          << (ed.cursorLine + 1) << ", Column " << (ed.cursorCol + 1) << ").\n";
            }
            else
            {
                std::cout << "Error: bookmark '" << bmName << "' not found.\n";
            }
        }
        else if (cmd == "bm_del")
        {
            std::string bmName = trim(readPrompt("Bookmark name to remove: "));
            if (removeBookmark(ed, bmName))
            {
                std::cout << "Bookmark '" << bmName << "' removed.\n";
            }
            else
            {
                std::cout << "Error: bookmark '" << bmName << "' not found.\n";
            }
        }
        else if (cmd == "format")
        {
            int lineNum = 0, startCol = 0, endCol = 0;
            if (readIntPrompt("Line (1-based): ", lineNum) &&
                readIntPrompt("Start column (1-based, inclusive): ", startCol) &&
                readIntPrompt("End column (1-based, exclusive): ", endCol))
            {
                int lineIdx = lineNum - 1;
                int sCol = startCol - 1;
                int eCol = endCol - 1;
                if (!isValidLine(ed, lineIdx) || !isValidColumnRange(ed, lineIdx, sCol, eCol) || sCol == eCol)
                {
                    std::cout << "Error: invalid line or column range.\n";
                    continue;
                }
                std::string style = trim(readPrompt("Style (bold / italic / both): "));
                FormatSpan span;
                span.startCol = sCol;
                span.endCol = eCol;
                if (style == "bold")
                {
                    span.type = BOLD;
                }
                else if (style == "italic")
                {
                    span.type = ITALIC;
                }
                else if (style == "both")
                {
                    span.type = BOLD_ITALIC;
                }
                else
                {
                    std::cout << "Error: unknown style (use bold, italic, or both).\n";
                    continue;
                }
                Line *target = ed.getLine(lineIdx);
                if (target != nullptr)
                {
                    target->formats.pushBack(span);
                    ed.setModified(true);
                    std::cout << "Formatting applied.\n";
                }
            }
        }
        else if (cmd == "clearfmt")
        {
            int lineNum = 0;
            if (readIntPrompt("Line (1-based): ", lineNum))
            {
                Line *target = ed.getLine(lineNum - 1);
                if (target == nullptr)
                {
                    std::cout << "Error: line " << lineNum << " does not exist.\n";
                }
                else
                {
                    target->formats.clear();
                    ed.setModified(true);
                    std::cout << "Formatting cleared on line " << lineNum << ".\n";
                }
            }
        }
        else
        {
            std::cout << "Unknown command '" << cmd << "'. Type 'help' to see available commands.\n";
        }
    }

    return 0;
}