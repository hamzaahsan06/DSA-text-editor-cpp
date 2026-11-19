#ifndef EDITOR_APP_H
#define EDITOR_APP_H

#include <string>
#include <istream>
#include "editor/Editor.h"
#include "editor/CommandParser.h"

// The command-driven console editor. Owns one Editor, an input stream
// (defaults to std::cin), and a message buffer that becomes the on-screen
// "Message:" text after each command.
class EditorApp
{
private:
    Editor ed;           // The document being edited.
    std::string message; // Text shown in the "Message:" area.
    bool running;        // Main loop keeps going while true.
    bool live;           // If true, the screen is cleared each redraw.
    std::istream *input; // Where getline() reads from. Defaults to std::cin.
    int windowLines;     // Max lines shown around the cursor.

    // Command handlers, one per command. Each receives the token list
    // so it can parse its arguments, and writes its result into `message`.
    void cmdNew(const DynamicArray<Token> &tok);
    void cmdOpen(const DynamicArray<Token> &tok);
    void cmdSave(const DynamicArray<Token> &tok);
    void cmdSaveAs(const DynamicArray<Token> &tok);
    void cmdExit(const DynamicArray<Token> &tok);
    void cmdInfo(const DynamicArray<Token> &tok);
    void cmdGoto(const DynamicArray<Token> &tok);
    void cmdAll(const DynamicArray<Token> &tok);
    void cmdLive(const DynamicArray<Token> &tok);
    void cmdHelp(const DynamicArray<Token> &tok);
    void cmdUndo(const DynamicArray<Token> &tok);
    void cmdRedo(const DynamicArray<Token> &tok);

    // Asks the user whether to save changes before a destructive command.
    // Returns true if it is safe to proceed (yes/no), false if cancelled.
    bool confirmDiscardChanges();

    // Clamps cursorLine/cursorCol so they point at a real position.
    void clampCursor();

public:
    // Sets up an editor with one blank line and the given input stream.
    EditorApp();

    // The main loop. Draws, prompts, reads one line, dispatches.
    void run();

    // Dispatches one command line. Used by run() and by the tests.
    void handleLine(const std::string &line);

    // Accessors for tests.
    Editor &getEditor();
    const std::string &getMessage() const;
    bool isRunning() const;
    void setLive(bool live);
    void setInput(std::istream &in);
};

#endif // EDITOR_APP_H