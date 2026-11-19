#include "editor/Screen.h"
#include "editor/Display.h"

#include <cstdlib> // std::system

// ---------- OutputCapture ----------

// Swaps std::cout's buffer with our private ostringstream.
OutputCapture::OutputCapture()
    : oldBuffer(std::cout.rdbuf(buffer.rdbuf()))
{
}

// Always restores the original std::cout buffer.
OutputCapture::~OutputCapture()
{
    std::cout.rdbuf(oldBuffer);
}

// Returns everything that was written to std::cout while the capture lived.
std::string OutputCapture::getText() const
{
    return buffer.str();
}

// ---------- clearScreen ----------

// Clears the console using the right command for the platform.
void clearScreen()
{
#ifdef _WIN32
    std::system("cls");
#else
    std::system("clear");
#endif
}

// ---------- Command table ----------

// One row in the help table: name, usage, description, example.
struct HelpEntry
{
    const char *name;
    const char *usage;
    const char *description;
    const char *example;
};

// Complete command table. Order matters: groups are printed top to bottom.
static const HelpEntry HELP_TABLE[] = {
    // FILE
    {"new", "new", "Start a new empty document.", "new"},
    {"open", "open <filename>", "Open a file. Asks if there are unsaved changes.", "open notes.txt"},
    {"save", "save", "Save the current document.", "save"},
    {"saveas", "saveas <filename>", "Save under a new file name.", "saveas copy.txt"},
    {"exit", "exit | quit | q", "Leave the editor. y=save, n=discard, c=cancel.", "exit"},
    // VIEW
    {"info", "info", "Show document statistics.", "info"},
    {"goto", "goto <line> [col]", "Move the cursor. col defaults to 1.", "goto 3 5"},
    {"all", "all", "Print the whole document.", "all"},
    {"live", "live on | live off", "Turn screen clearing on or off.", "live off"},
    {"help", "help [command]", "List commands, or show one command's usage.", "help ins"},
    // EDIT
    {"ins", "ins <line> <col> <text>", "Insert text. Quote it to keep leading/trailing spaces.", "ins 1 1 \" hi \""},
    {"add", "add <text>", "Insert text at the cursor.", "add hello"},
    {"nl", "nl [line col]", "Split a line / insert a new line.", "nl 2 4"},
    {"del", "del <line> <col>", "Delete one character.", "del 1 3"},
    {"delw", "delw <line> <col>", "Delete the word at that position.", "delw 2 5"},
    {"delr", "delr <line> <startCol> <endCol>", "Delete columns startCol..endCol (inclusive).", "delr 1 3 5"},
    {"dell", "dell <from> [to]", "Delete one line, or a range of lines.", "dell 2 4"},
    {"undo", "undo [n]", "Undo the last n edits (default 1).", "undo 3"},
    {"redo", "redo [n]", "Redo the last n undone edits (default 1).", "redo"},
    // SEARCH
    {"find", "find <text>", "Find matches; cursor jumps to the first.", "find hello"},
    {"replace", "replace <old> <new> <n>", "Replace the n-th match.", "replace cat dog 1"},
    {"replaceall", "replaceall <old> <new>", "Replace every match.", "replaceall a A"},
    // BOOKMARKS
    {"mark", "mark <name>", "Save a bookmark at the cursor.", "mark here"},
    {"marks", "marks", "List all bookmarks.", "marks"},
    {"jump", "jump <name>", "Jump to a bookmark.", "jump here"},
    {"unmark", "unmark <name>", "Remove a bookmark.", "unmark here"},
    {"back", "back", "Return to the previous position.", "back"},
};

static const int HELP_COUNT = (int)(sizeof(HELP_TABLE) / sizeof(HELP_TABLE[0]));

// Returns the entry whose name matches, or nullptr.
static const HelpEntry *findEntry(const std::string &name)
{
    for (int i = 0; i < HELP_COUNT; ++i)
    {
        const char *n = HELP_TABLE[i].name;
        // Compare manually to avoid pulling in <cstring> for strcmp.
        int j = 0;
        while (n[j] != '\0' && name[j] != '\0' && n[j] == name[j])
            ++j;
        if (n[j] == '\0' && name[j] == '\0')
            return &HELP_TABLE[i];
    }
    return nullptr;
}

// ---------- drawScreen ----------

// Draws the whole screen in `out`.
void drawScreen(const Editor &ed,
                const std::string &message,
                bool live,
                int windowLines,
                std::ostream &out)
{
    // Live mode clears the screen; offline mode prints a visual separator.
    if (live)
    {
        clearScreen();
    }
    else
    {
        out << "================================================================\n";
    }

    // Title.
    out << "=== Text Editor  (type 'help') ===\n";

    // Status bar (from Display).
    displayStatusBar(ed, out);

    // Separator.
    out << "----------------------------------------------------------------\n";

    // Document window with the cursor line marked.
    displayWindow(ed, windowLines, out);

    // Separator.
    out << "----------------------------------------------------------------\n";

    // Message: printed even when empty (shows the empty prompt area).
    out << "Message: ";
    if (!message.empty())
    {
        out << message;
        if (message[message.length() - 1] != '\n')
            out << '\n';
    }
    else
    {
        out << '\n';
    }
}

// ---------- Help / usage ----------

// Groups used by printHelp. Order in the group list controls print order.
static const char *GROUP_NAMES[] = {"FILE", "VIEW", "EDIT", "SEARCH", "BOOKMARKS"};
// Each entry is the index in HELP_TABLE at which that group begins (inclusive).
static const int GROUP_STARTS[] = {0, 5, 10, 19, 22};
static const int GROUP_ENDS[] = {4, 9, 18, 21, 26}; // inclusive

// Prints all commands grouped, or one command's usage + description + example.
void printHelp(std::ostream &out, const std::string &topic)
{
    if (topic.empty())
    {
        out << "Available commands:\n";
        for (int g = 0; g < 5; ++g)
        {
            out << "\n"
                << GROUP_NAMES[g] << "\n";
            for (int i = GROUP_STARTS[g]; i <= GROUP_ENDS[g]; ++i)
            {
                // Pad usage to a fixed width for aligned output.
                std::string u = HELP_TABLE[i].usage;
                while (u.length() < 34)
                    u += ' ';
                out << "  " << u << " " << HELP_TABLE[i].description << "\n";
            }
        }
        return;
    }

    const HelpEntry *e = findEntry(topic);
    if (e == nullptr)
    {
        out << "No help available for '" << topic << "'. Try 'help' to list commands.\n";
        return;
    }

    out << "Usage:       " << e->usage << "\n";
    out << "Description: " << e->description << "\n";
    out << "Example:     " << e->example << "\n";
}

// Returns "Usage: ins <line> <col> <text>", or "" for unknown commands.
std::string getUsage(const std::string &command)
{
    const HelpEntry *e = findEntry(command);
    if (e == nullptr)
        return "";
    std::string u = "Usage: ";
    u += e->usage;
    return u;
}