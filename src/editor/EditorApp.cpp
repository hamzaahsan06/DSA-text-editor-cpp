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

// True if the string contains a space, tab, newline or carriage return.
static bool hasWhitespace(const std::string &s)
{
    for (int i = 0; i < (int)s.length(); ++i)
    {
        char c = s[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
            return true;
    }
    return false;
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
        cmdOpen(line, tokens);
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
    else if (cmd == "ins")
        cmdIns(line, tokens);
    else if (cmd == "add")
        cmdAdd(line, tokens);
    else if (cmd == "nl")
        cmdNl(tokens);
    else if (cmd == "del")
        cmdDel(tokens);
    else if (cmd == "delw")
        cmdDelw(tokens);
    else if (cmd == "delr")
        cmdDelr(tokens);
    else if (cmd == "dell")
        cmdDell(tokens);
    else if (cmd == "find")
        cmdFind(line, tokens);
    else if (cmd == "replace")
        cmdReplace(tokens);
    else if (cmd == "replaceall")
        cmdReplaceAll(tokens);
    else if (cmd == "mark")
        cmdMark(tokens);
    else if (cmd == "marks")
        cmdMarks(tokens);
    else if (cmd == "jump")
        cmdJump(tokens);
    else if (cmd == "unmark")
        cmdUnmark(tokens);
    else if (cmd == "back")
        cmdBack(tokens);
    else
    {
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

// ----- FILE handlers -----

// new: discard any unsaved changes, then reset the document.
void EditorApp::cmdNew(const DynamicArray<Token> &tok)
{
    (void)tok;
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

// open <filename>: filename is the rest of the line, so spaces work.
void EditorApp::cmdOpen(const std::string &line, const DynamicArray<Token> &tok)
{
    if (tok.size() < 2)
    {
        message = appUsage("open");
        return;
    }
    std::string filename = restOfLine(line, tok, 1);
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

// ----- VIEW handlers -----

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

    (void)savePreviousPosition(ed);
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

// ----- HISTORY handlers -----

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

// ----- EDIT handlers -----

// ins <line> <col> <text>: text is the rest of the line, so spaces are kept.
void EditorApp::cmdIns(const std::string &line, const DynamicArray<Token> &tok)
{
    if (tok.size() < 3)
    {
        message = appUsage("ins");
        return;
    }

    int line1 = 0;
    int col1 = 0;
    Token t;
    tok.get(1, t);
    if (!parsePositiveInt(t.text, line1))
    {
        message = appUsage("ins");
        return;
    }
    tok.get(2, t);
    if (!parsePositiveInt(t.text, col1))
    {
        message = appUsage("ins");
        return;
    }

    std::string text = restOfLine(line, tok, 3);
    if (text.empty())
    {
        message = appUsage("ins");
        return;
    }

    int line0 = line1 - 1;
    int col0 = col1 - 1;
    int count = (int)text.length();

    OutputCapture cap;
    bool ok = insertText(ed, line0, col0, text);
    std::string captured = cap.getText();
    if (ok)
    {
        message = "Inserted " + intToString(count) + " character(s) at line " + intToString(line1) + ", column " + intToString(col1) + ".";
    }
    else if (!captured.empty())
    {
        message = captured;
    }
    else
    {
        message = "Insert failed.";
    }
    clampCursor();
}

// add <text>: insert text at the cursor. Empty doc uses (0, 0).
void EditorApp::cmdAdd(const std::string &line, const DynamicArray<Token> &tok)
{
    if (tok.size() < 2)
    {
        message = appUsage("add");
        return;
    }

    std::string text = restOfLine(line, tok, 1);
    if (text.empty())
    {
        message = appUsage("add");
        return;
    }

    int line0 = 0;
    int col0 = 0;
    if (!ed.document.isEmpty())
    {
        clampCursor();
        line0 = ed.cursorLine;
        col0 = ed.cursorCol;
    }

    int count = (int)text.length();
    int showLine = line0 + 1;
    int showCol = col0 + 1;

    OutputCapture cap;
    bool ok = insertText(ed, line0, col0, text);
    std::string captured = cap.getText();
    if (ok)
    {
        message = "Inserted " + intToString(count) + " character(s) at line " + intToString(showLine) + ", column " + intToString(showCol) + ".";
    }
    else if (!captured.empty())
    {
        message = captured;
    }
    else
    {
        message = "Insert failed.";
    }
    clampCursor();
}

// nl [line col]: split a line. No args = split at the cursor.
void EditorApp::cmdNl(const DynamicArray<Token> &tok)
{
    if (tok.size() != 1 && tok.size() != 3)
    {
        message = appUsage("nl");
        return;
    }

    int line0 = 0;
    int col0 = 0;
    if (tok.size() == 3)
    {
        int line1 = 0;
        int col1 = 0;
        Token t;
        tok.get(1, t);
        if (!parsePositiveInt(t.text, line1))
        {
            message = appUsage("nl");
            return;
        }
        tok.get(2, t);
        if (!parsePositiveInt(t.text, col1))
        {
            message = appUsage("nl");
            return;
        }
        line0 = line1 - 1;
        col0 = col1 - 1;
    }
    else
    {
        if (!ed.document.isEmpty())
        {
            clampCursor();
            line0 = ed.cursorLine;
            col0 = ed.cursorCol;
        }
    }

    OutputCapture cap;
    bool ok = insertNewLine(ed, line0, col0);
    std::string captured = cap.getText();
    if (ok)
    {
        message = "New line inserted.";
    }
    else if (!captured.empty())
    {
        message = captured;
    }
    else
    {
        message = "Insert new line failed.";
    }
    clampCursor();
}

// del <line> <col>: delete one character.
void EditorApp::cmdDel(const DynamicArray<Token> &tok)
{
    if (tok.size() != 3)
    {
        message = appUsage("del");
        return;
    }

    int line1 = 0;
    int col1 = 0;
    Token t;
    tok.get(1, t);
    if (!parsePositiveInt(t.text, line1))
    {
        message = appUsage("del");
        return;
    }
    tok.get(2, t);
    if (!parsePositiveInt(t.text, col1))
    {
        message = appUsage("del");
        return;
    }

    OutputCapture cap;
    bool ok = deleteChar(ed, line1 - 1, col1 - 1);
    std::string captured = cap.getText();
    if (ok)
    {
        message = "Deleted 1 character.";
    }
    else if (!captured.empty())
    {
        message = captured;
    }
    else
    {
        message = "Delete failed.";
    }
    clampCursor();
}

// delw <line> <col>: delete the word at (line, col).
void EditorApp::cmdDelw(const DynamicArray<Token> &tok)
{
    if (tok.size() != 3)
    {
        message = appUsage("delw");
        return;
    }

    int line1 = 0;
    int col1 = 0;
    Token t;
    tok.get(1, t);
    if (!parsePositiveInt(t.text, line1))
    {
        message = appUsage("delw");
        return;
    }
    tok.get(2, t);
    if (!parsePositiveInt(t.text, col1))
    {
        message = appUsage("delw");
        return;
    }

    OutputCapture cap;
    bool ok = deleteWord(ed, line1 - 1, col1 - 1);
    std::string captured = cap.getText();
    if (ok)
    {
        message = "Deleted word.";
    }
    else if (!captured.empty())
    {
        message = captured;
    }
    else
    {
        message = "Delete word failed.";
    }
    clampCursor();
}

// delr <line> <startCol> <endCol>: inclusive range.
void EditorApp::cmdDelr(const DynamicArray<Token> &tok)
{
    if (tok.size() != 4)
    {
        message = appUsage("delr");
        return;
    }

    int line1 = 0;
    int start1 = 0;
    int end1 = 0;
    Token t;
    tok.get(1, t);
    if (!parsePositiveInt(t.text, line1))
    {
        message = appUsage("delr");
        return;
    }
    tok.get(2, t);
    if (!parsePositiveInt(t.text, start1))
    {
        message = appUsage("delr");
        return;
    }
    tok.get(3, t);
    if (!parsePositiveInt(t.text, end1))
    {
        message = appUsage("delr");
        return;
    }

    if (start1 > end1)
    {
        message = "Error: start column must not be after end column.";
        return;
    }

    int count = end1 - start1 + 1;

    OutputCapture cap;
    bool ok = deleteText(ed, line1 - 1, start1 - 1, end1);
    std::string captured = cap.getText();
    if (ok)
    {
        message = "Deleted " + intToString(count) + " character(s).";
    }
    else if (!captured.empty())
    {
        message = captured;
    }
    else
    {
        message = "Delete range failed.";
    }
    clampCursor();
}

// dell <from> [to]: delete one line or an inclusive range.
void EditorApp::cmdDell(const DynamicArray<Token> &tok)
{
    if (tok.size() != 2 && tok.size() != 3)
    {
        message = appUsage("dell");
        return;
    }

    int from1 = 0;
    int to1 = 0;
    Token t;
    tok.get(1, t);
    if (!parsePositiveInt(t.text, from1))
    {
        message = appUsage("dell");
        return;
    }
    if (tok.size() == 3)
    {
        tok.get(2, t);
        if (!parsePositiveInt(t.text, to1))
        {
            message = appUsage("dell");
            return;
        }
    }
    else
    {
        to1 = from1;
    }

    if (from1 > to1)
    {
        message = "Error: start line must not be after end line.";
        return;
    }

    int count = to1 - from1 + 1;

    OutputCapture cap;
    bool ok = deleteLines(ed, from1 - 1, to1 - 1);
    std::string captured = cap.getText();
    if (ok)
    {
        message = "Deleted " + intToString(count) + " line(s).";
    }
    else if (!captured.empty())
    {
        message = captured;
    }
    else
    {
        message = "Delete lines failed.";
    }
    clampCursor();
}

// ----- SEARCH handlers -----

// find <text>: shows at most 10 matches, then jumps to the first.
void EditorApp::cmdFind(const std::string &line, const DynamicArray<Token> &tok)
{
    if (tok.size() < 2)
    {
        message = "Error: nothing to search for.";
        return;
    }
    if (ed.document.isEmpty())
    {
        message = "Error: the document is empty.";
        return;
    }

    std::string text = restOfLine(line, tok, 1);
    if (text.empty())
    {
        message = "Error: nothing to search for.";
        return;
    }

    DynamicArray<Position> hits = searchAll(ed, text);
    int total = hits.size();
    if (total == 0)
    {
        message = "Not found.";
        return;
    }

    int shown = total;
    if (shown > 10)
        shown = 10;

    std::string out = "Found " + intToString(total) + " occurrence(s):\n";
    for (int i = 0; i < shown; ++i)
    {
        Position p;
        hits.get(i, p);
        out += "Line " + intToString(p.line + 1) + ", Column " + intToString(p.col + 1) + "\n";
    }
    if (total > 10)
    {
        out += "... and " + intToString(total - 10) + " more.\n";
    }

    (void)savePreviousPosition(ed);
    Position first;
    hits.get(0, first);
    ed.cursorLine = first.line;
    ed.cursorCol = first.col;
    clampCursor();

    message = out;
}

// replace <old> <new> <n>: replace the n-th match (1-based).
void EditorApp::cmdReplace(const DynamicArray<Token> &tok)
{
    if (tok.size() != 4)
    {
        message = appUsage("replace");
        return;
    }
    if (ed.document.isEmpty())
    {
        message = "Error: the document is empty.";
        return;
    }

    Token t;
    tok.get(1, t);
    std::string oldText = t.text;
    tok.get(2, t);
    std::string newText = t.text;

    if (oldText.empty())
    {
        message = "Error: nothing to search for.";
        return;
    }

    int n = 0;
    tok.get(3, t);
    if (!parsePositiveInt(t.text, n))
    {
        message = appUsage("replace");
        return;
    }

    DynamicArray<Position> hits = searchAll(ed, oldText);
    int total = hits.size();
    if (total == 0)
    {
        message = "Not found.";
        return;
    }
    if (n > total)
    {
        message = "Error: only " + intToString(total) + " occurrence(s) found.";
        return;
    }

    Position where;
    hits.get(n - 1, where);

    OutputCapture cap;
    bool ok = replaceOccurrence(ed, oldText, newText, n);
    std::string captured = cap.getText();
    if (ok)
    {
        message = "Replaced occurrence " + intToString(n) + " at line " + intToString(where.line + 1) + ", column " + intToString(where.col + 1) + ".";
    }
    else if (!captured.empty())
    {
        message = captured;
    }
    else
    {
        message = "Replace failed.";
    }
    clampCursor();
}

// replaceall <old> <new>: one undo step for the whole replacement.
void EditorApp::cmdReplaceAll(const DynamicArray<Token> &tok)
{
    if (tok.size() != 3)
    {
        message = appUsage("replaceall");
        return;
    }
    if (ed.document.isEmpty())
    {
        message = "Error: the document is empty.";
        return;
    }

    Token t;
    tok.get(1, t);
    std::string oldText = t.text;
    tok.get(2, t);
    std::string newText = t.text;

    if (oldText.empty())
    {
        message = "Error: nothing to search for.";
        return;
    }

    DynamicArray<Position> hits = searchAll(ed, oldText);
    if (hits.size() == 0)
    {
        message = "Not found.";
        return;
    }

    OutputCapture cap;
    int count = replaceAll(ed, oldText, newText);
    std::string captured = cap.getText();
    if (count == 0)
    {
        if (!captured.empty())
            message = captured;
        else
            message = "Not found.";
    }
    else
    {
        message = "Replaced " + intToString(count) + " occurrence(s). (one undo step)";
    }
    clampCursor();
}

// ----- BOOKMARK handlers -----

// mark <name>: save a bookmark at the cursor.
void EditorApp::cmdMark(const DynamicArray<Token> &tok)
{
    if (tok.size() != 2)
    {
        message = appUsage("mark");
        return;
    }
    if (ed.document.isEmpty())
    {
        message = "Error: the document is empty.";
        return;
    }

    Token t;
    tok.get(1, t);
    std::string name = t.text;

    if (name.empty())
    {
        message = appUsage("mark");
        return;
    }
    if (name.length() >= 2 && name[0] == '_' && name[1] == '_')
    {
        message = "Error: names starting with '__' are reserved.";
        return;
    }
    if (hasWhitespace(name))
    {
        message = "Error: bookmark names cannot contain spaces.";
        return;
    }
    if (findBookmark(ed, name) != -1)
    {
        message = "A bookmark named " + name + " already exists.";
        return;
    }

    clampCursor();
    int line0 = ed.cursorLine;
    int col0 = ed.cursorCol;

    bool ok = addBookmarkAtCursor(ed, name);
    if (ok)
    {
        message = "Bookmark " + name + " set at line " + intToString(line0 + 1) + ", column " + intToString(col0 + 1) + ".";
    }
    else
    {
        message = "Error: could not set bookmark.";
    }
}

// marks: list bookmarks in the message area.
void EditorApp::cmdMarks(const DynamicArray<Token> &tok)
{
    if (tok.size() != 1)
    {
        message = appUsage("marks");
        return;
    }
    OutputCapture cap;
    listBookmarks(ed, std::cout);
    message = cap.getText();
}

// jump <name>: save the current position, then move to the named bookmark.
void EditorApp::cmdJump(const DynamicArray<Token> &tok)
{
    if (tok.size() != 2)
    {
        message = appUsage("jump");
        return;
    }
    if (ed.document.isEmpty())
    {
        message = "Error: the document is empty.";
        return;
    }

    Token t;
    tok.get(1, t);
    std::string name = t.text;

    int index = findBookmark(ed, name);
    if (index == -1)
    {
        message = "Bookmark " + name + " not found.";
        return;
    }
    if (name.length() >= 2 && name[0] == '_' && name[1] == '_')
    {
        message = "Bookmark " + name + " not found.";
        return;
    }

    Bookmark target;
    if (!ed.bookmarks.get(index, target))
    {
        message = "Bookmark " + name + " not found.";
        return;
    }

    (void)savePreviousPosition(ed);
    bool ok = gotoBookmark(ed, name);
    if (ok)
    {
        message = "Jumped to " + name + " (line " + intToString(target.line + 1) + ", column " + intToString(target.col + 1) + ").";
    }
    else
    {
        message = "Error: could not jump to " + name + ".";
    }
    clampCursor();
}

// unmark <name>: remove a bookmark.
void EditorApp::cmdUnmark(const DynamicArray<Token> &tok)
{
    if (tok.size() != 2)
    {
        message = appUsage("unmark");
        return;
    }

    Token t;
    tok.get(1, t);
    std::string name = t.text;

    if (findBookmark(ed, name) == -1)
    {
        message = "Bookmark " + name + " not found.";
        return;
    }
    bool ok = removeBookmark(ed, name);
    if (ok)
        message = "Bookmark " + name + " removed.";
    else
        message = "Error: could not remove " + name + ".";
}

// back: return to the saved previous position.
void EditorApp::cmdBack(const DynamicArray<Token> &tok)
{
    if (tok.size() != 1)
    {
        message = appUsage("back");
        return;
    }

    bool ok = returnToPreviousPosition(ed);
    if (!ok)
    {
        message = "No previous position saved.";
        return;
    }

    clampCursor();
    message = "Returned to line " + intToString(ed.cursorLine + 1) + ", column " + intToString(ed.cursorCol + 1) + ".";
}