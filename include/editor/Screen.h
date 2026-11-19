#ifndef EDITOR_SCREEN_H
#define EDITOR_SCREEN_H

#include <string>
#include <iostream>
#include <sstream>
#include "editor/Editor.h"

// OutputCapture
// -------------
// Existing modules print their own errors with std::cout. On a live screen
// those writes would be erased immediately on the next redraw. So EditorApp
// runs each module call inside an OutputCapture: the constructor swaps
// cout's buffer with a private ostringstream, so every std::cout write
// lands inside this object. Then EditorApp pulls the captured text out via
// getText() and shows it as the on-screen "Message:". The destructor always
// restores the original cout buffer, so normal printing resumes afterwards.
//
// Copying is disabled: two OutputCaptures fighting over the same cout buffer
// would restore the wrong pointer on scope exit.
class OutputCapture
{
private:
    std::ostringstream buffer; // Where std::cout writes while the capture lives.
    std::streambuf *oldBuffer; // cout's original buffer to restore later.

    OutputCapture(const OutputCapture &);            // disabled: no body needed
    OutputCapture &operator=(const OutputCapture &); // disabled

public:
    OutputCapture();  // swaps cout's buffer with `buffer`
    ~OutputCapture(); // restores cout's original buffer

    std::string getText() const; // returns everything captured so far
};

// Clears the console. The ONLY #ifdef _WIN32 in the project.
void clearScreen();

// Draws the whole live screen in `out`.
// live == true  -> calls clearScreen() first, then prints the layout.
// live == false -> prints a line of '=' characters as a separator instead.
// Always prints: title, status bar, separator, document window, separator,
// and "Message: ..." (multi-line messages are printed as-is).
// Does NOT print the prompt — EditorApp does that after drawScreen returns.
void drawScreen(const Editor &ed,
                const std::string &message,
                bool live,
                int windowLines,
                std::ostream &out);

// Prints the command table (topic == "") or a single command's usage/example.
// Unknown topic prints a friendly message.
void printHelp(std::ostream &out, const std::string &topic);

// Returns "Usage: ins <line> <col> <text>" for a known command, "" otherwise.
std::string getUsage(const std::string &command);

#endif // EDITOR_SCREEN_H