#include "editor/EditorApp.h"
#include "editor/CommandParser.h"
#include "editor/Screen.h"
#include "editor/Display.h"
#include "editor/FileManager.h"
#include "editor/TextOps.h"
#include "editor/SearchReplace.h"
#include "editor/Bookmarks.h"
#include "editor/UndoRedo.h"
#include "editor/Validation.h"
#include "editor/Formatting.h"
#include <iostream>
#include <string>

// ----- helpers -----

// Converts an int to a string using a plain loop (no std::to_string).
static std::string intToString(int value)
{
    if (value == 0)
        return "0";
    bool negative = false;
    if (value < 0)
    {
        negative = true;
        value = -value;
    }
    std::string digits;
    while (value > 0)
    {
        digits = (char)('0' + (value % 10)) + digits;
        value /= 10;
    }
    if (negative)
        digits = "-" + digits;
    return digits;
}

// A Command struct for the app's own table (unrelated to UndoRedo's Command).
struct AppCmd
{
    const char *name;
    const char *usage;
};

// The complete command list. Kept here so "unknown command" can print the
// closest match and so the "not implemented yet" list is stable.
static const AppCmd APP_COMMANDS[] = {
    {"new", "new"},
    {"open", "open <filename>"},
    {"save", "save"},
    {"saveas", "saveas <filename>"},
    {"exit", "exit | quit | q"},
    {"quit", "exit | quit | q"},
    {"q", "exit | quit | q"},
    {"info", "info"},
    {"goto", "goto <line> [col]"},
    {"all", "all"},
    {"live", "live on | live off"},
    {"help", "help [command]"},
    {"ins", "ins <line> <col> <text>"},
    {"add", "add <text>"},
    {"nl", "nl [line col]"},
    {"del", "del <line> <col>"},
    {"delw", "delw <line> <col>"},
    {"delr", "delr <line> <startCol> <endCol>"},
    {"dell", "dell <from> [to]"},
    {"undo", "undo [n]"},
    {"redo", "redo [n]"},
    {"find", "find <text>"},
    {"replace", "replace <old> <new> <n>"},
    {"replaceall", "replaceall <old> <new>"},
    {"mark", "mark <name>"},
    {"marks", "marks"},
    {"jump", "jump <name>"},
    {"unmark", "unmark <name>"},
    {"back", "back"},
};
static const int APP_CMD_COUNT = (int)(sizeof(APP_COMMANDS) / sizeof(APP_COMMANDS[0]));

// Returns the usage string for the given command, or "" if unknown.
static std::string appUsage(const std::string &name)
{
    for (int i = 0; i < APP_CMD_COUNT; ++i)
    {
        if (name == APP_COMMANDS[i].name)
        {
            return std::string("Usage: ") + APP_COMMANDS[i].usage;
        }
    }
    return "";
}

// ----- constructor / accessors -----

EditorApp::EditorApp()
    : message(""), running(true), live(true), input(&std::cin), windowLines(20)
{
    clampCursor();
}

Editor &EditorApp::getEditor() { return ed; }
const std::string &EditorApp::getMessage() const { return message; }
bool EditorApp::isRunning() const { return running; }
void EditorApp::setLive(bool liveVal) { live = liveVal; }
void EditorApp::setInput(std::istream &in) { input = &in; }

// ----- helpers -----

// Clamps cursor to a valid position inside the document.
void EditorApp::clampCursor()
{
    if (ed.document.isEmpty())
    {
        ed.cursorLine = 0;
        ed.cursorCol = 0;
        return;
    }
    if (ed.cursorLine < 0)
        ed.cursorLine = 0;
    if (ed.cursorLine >= ed.getLineCount())
        ed.cursorLine = ed.getLineCount() - 1;
    Line *l = ed.getLine(ed.cursorLine);
    if (l != nullptr)
    {
        if (ed.cursorCol < 0)
            ed.cursorCol = 0;
        if (ed.cursorCol > (int)l->text.length())
            ed.cursorCol = (int)l->text.length();
    }
}

// Asks the user before discarding changes. Returns true if we may proceed.
bool EditorApp::confirmDiscardChanges()
{
    if (!ed.isModified())
        return true;

    while (true)
    {
        std::cout << "You have unsaved changes. Save first? (y = save, n = discard, c = cancel): ";
        std::string ans;
        if (!std::getline(*input, ans))
        {
            std::cout << "\n";
            return false; // input ended; treat as cancel
        }
        std::string lower = toLowerCase(ans);
        if (lower == "y")
        {
            if (ed.getFilename().empty())
            {
                std::cout << "File name: ";
                std::string fname;
                if (!std::getline(*input, fname) || fname.empty())
                {
                    std::cout << "Cancelled (no file name).\n";
                    return false;
                }
                if (!saveFileAs(ed, fname))
                {
                    std::cout << "Save failed.\n";
                    return false;
                }
            }
            else
            {
                if (!saveFile(ed))
                {
                    std::cout << "Save failed.\n";
                    return false;
                }
            }
            return true;
        }
        if (lower == "n")
            return true;
        if (lower == "c")
            return false;
        std::cout << "Please answer y, n or c.\n";
    }
}

// ----- run -----

void EditorApp::run()
{
    while (running)
    {
        drawScreen(ed, message, live, windowLines, std::cout);
        std::cout << "> ";
        std::string line;
        if (!std::getline(*input, line))
        {
            running = false;
            break;
        }
        handleLine(line);
    }
}

// ----- handleLine -----

void EditorApp::handleLine(const std::string &line)
{
    message = "";

    // Split and ignore an entirely blank line.
    DynamicArray<Token> tokens = splitLine(line);
    if (tokens.size() == 0)
        return;

    Token first;
    tokens.get(0, first);
    std::string cmd = toLowerCase(first.text);

    if (cmd == "new")
        cmdNew(tokens);
    else if (cmd == "open")
        cmdOpen(tokens);
    else if (cmd == "save")
        cmdSave(tokens);
    else if (cmd == "saveas")
        cmdSaveAs(tokens);
    else if (cmd == "exit" ||
             cmd == "quit" ||
             cmd == "q")
        cmdExit(tokens);
    else if (cmd == "info")
        cmdInfo(tokens);
    else if (cmd == "goto")
        cmdGoto(tokens);
    else if (cmd == "all")
        cmdAll(tokens);
    else if (cmd == "live")
        cmdLive(tokens);
    else if (cmd == "help")
        cmdHelp(tokens);
    else if (cmd == "undo")
        cmdUndo(tokens);
    else if (cmd == "redo")
        cmdRedo(tokens);
    else
    {
        // Not one of the currently-implemented commands.
        std::string known = appUsage(cmd);
        if (!known.empty())
        {
            message = "Not implemented yet.  " + known;
        }
        else
        {
            message = "Unknown command '" + first.text + "'. Type 'help' for the list.";
        }
    }
}

// ----- one handler per command -----

// new: discard any unsaved changes, then reset the document.
void EditorApp::cmdNew(const DynamicArray<Token> &tok)
{
    (void)tok; // no arguments
    if (!confirmDiscardChanges())
    {
        message = "Cancelled.";
        return;
    }
    OutputCapture cap;
    newDocument(ed);
    std::string captured = cap.getText();
    message = captured.empty() ? "New document." : captured;
    clampCursor();
}

// open <filename>: discard changes, then load from disk.
void EditorApp::cmdOpen(const DynamicArray<Token> &tok)
{
    if (tok.size() < 2)
    {
        message = appUsage("open");
        return;
    }
    std::string filename = restOfLine(*(new std::string()), tok, 1); // not used
    // Actually use restOfLine against the original line? We don't have it here.
    // Simpler: use token 1's text (quoted names lose their quotes in splitLine).
    Token t;
    tok.get(1, t);
    filename = t.text;
    if (filename.empty())
    {
        message = appUsage("open");
        return;
    }
    if (!confirmDiscardChanges())
    {
        message = "Cancelled.";
        return;
    }
    OutputCapture cap;
    bool ok = openFile(ed, filename);
    std::string captured = cap.getText();
    if (ok)
        message = "Opened " + filename + ".";
    else if (!captured.empty())
        message = captured;
    else
        message = "Could not open '" + filename + "'.";
    clampCursor();
}

// save: write to the current filename, or complain.
void EditorApp::cmdSave(const DynamicArray<Token> &tok)
{
    (void)tok;
    OutputCapture cap;
    bool ok = saveFile(ed);
    std::string captured = cap.getText();
    if (ok)
        message = "Saved " + (ed.getFilename().empty() ? std::string("(no name)") : ed.getFilename()) + ".";
    else if (!captured.empty())
        message = captured;
    else
        message = "Save failed.";
}

// saveas <filename>: write under a new name.
void EditorApp::cmdSaveAs(const DynamicArray<Token> &tok)
{
    if (tok.size() < 2)
    {
        message = appUsage("saveas");
        return;
    }
    Token t;
    tok.get(1, t);
    std::string filename = t.text;
    if (filename.empty())
    {
        message = appUsage("saveas");
        return;
    }
    OutputCapture cap;
    bool ok = saveFileAs(ed, filename);
    std::string captured = cap.getText();
    if (ok)
        message = "Saved " + filename + ".";
    else if (!captured.empty())
        message = captured;
    else
        message = "Save failed.";
}

// exit / quit / q: leave the loop after a save check.
void EditorApp::cmdExit(const DynamicArray<Token> &tok)
{
    (void)tok;
    if (!confirmDiscardChanges())
    {
        message = "Cancelled.";
        return;
    }
    running = false;
}

// info: capture the full stats block.
void EditorApp::cmdInfo(const DynamicArray<Token> &tok)
{
    (void)tok;
    OutputCapture cap;
    displayStats(ed, std::cout);
    message = cap.getText();
}

// goto <line> [col]: move the cursor, saving the old position first.
void EditorApp::cmdGoto(const DynamicArray<Token> &tok)
{
    if (ed.document.isEmpty())
    {
        message = "Cannot goto: document is empty.";
        return;
    }
    if (tok.size() < 2 || tok.size() > 3)
    {
        message = appUsage("goto");
        return;
    }

    int line1 = 0;
    Token t;
    tok.get(1, t);
    if (!parsePositiveInt(t.text, line1))
    {
        message = appUsage("goto");
        return;
    }
    if (line1 > ed.getLineCount())
    {
        message = "Line " + intToString(line1) + " is out of range.";
        return;
    }

    int col1 = 1;
    if (tok.size() == 3)
    {
        tok.get(2, t);
        if (!parsePositiveInt(t.text, col1))
        {
            message = appUsage("goto");
            return;
        }
    }
    Line *targetLine = ed.getLine(line1 - 1);
    int lineLen = targetLine ? (int)targetLine->text.length() : 0;
    if (col1 > lineLen + 1)
    {
        message = "Column " + intToString(col1) + " is out of range.";
        return;
    }

    (void)savePreviousPosition(ed); // ignore result
    ed.cursorLine = line1 - 1;
    ed.cursorCol = col1 - 1;
    clampCursor();
    message = "Cursor at line " + intToString(ed.cursorLine + 1) + ", column " + intToString(ed.cursorCol + 1) + ".";
}

// all: capture the whole document into the message.
void EditorApp::cmdAll(const DynamicArray<Token> &tok)
{
    (void)tok;
    OutputCapture cap;
    displayDocument(ed, std::cout);
    message = cap.getText();
}

// live on / live off
void EditorApp::cmdLive(const DynamicArray<Token> &tok)
{
    if (tok.size() != 2)
    {
        message = appUsage("live");
        return;
    }
    Token t;
    tok.get(1, t);
    std::string mode = toLowerCase(t.text);
    if (mode == "on")
    {
        live = true;
        message = "Live screen ON.";
        return;
    }
    if (mode == "off")
    {
        live = false;
        message = "Live screen OFF.";
        return;
    }
    message = appUsage("live");
}

// help [command]
void EditorApp::cmdHelp(const DynamicArray<Token> &tok)
{
    std::string topic;
    if (tok.size() >= 2)
    {
        Token t;
        tok.get(1, t);
        topic = toLowerCase(t.text);
    }
    OutputCapture cap;
    printHelp(std::cout, topic);
    message = cap.getText();
}

// undo [n]
void EditorApp::cmdUndo(const DynamicArray<Token> &tok)
{
    int n = 1;
    if (tok.size() >= 2)
    {
        Token t;
        tok.get(1, t);
        if (!parsePositiveInt(t.text, n))
        {
            message = appUsage("undo");
            return;
        }
    }
    if (tok.size() > 2)
    {
        message = appUsage("undo");
        return;
    }

    UndoRedo ur;
    int done = 0;
    for (int i = 0; i < n; ++i)
    {
        if (!ur.undo(ed))
            break;
        ++done;
    }
    if (done == 0)
        message = "Nothing to undo.";
    else
        message = "Undid " + intToString(done) + " operation(s).";
    clampCursor();
}

// redo [n]
void EditorApp::cmdRedo(const DynamicArray<Token> &tok)
{
    int n = 1;
    if (tok.size() >= 2)
    {
        Token t;
        tok.get(1, t);
        if (!parsePositiveInt(t.text, n))
        {
            message = appUsage("redo");
            return;
        }
    }
    if (tok.size() > 2)
    {
        message = appUsage("redo");
        return;
    }

    UndoRedo ur;
    int done = 0;
    for (int i = 0; i < n; ++i)
    {
        if (!ur.redo(ed))
            break;
        ++done;
    }
    if (done == 0)
        message = "Nothing to redo.";
    else
        message = "Redid " + intToString(done) + " operation(s).";
    clampCursor();
}