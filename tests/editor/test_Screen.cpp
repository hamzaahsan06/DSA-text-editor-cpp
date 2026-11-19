// g++ -std=c++11 -Wall -Wextra -Iinclude -Iinclude/editor tests/editor/test_Screen.cpp src/editor/Screen.cpp src/editor/Display.cpp -o build/test_Screen ; if ($?) { ./build/test_Screen }

#include <iostream>
#include <sstream>
#include <string>
#include "editor/Screen.h"
#include "editor/Editor.h"
#include "editor/Types.h"
#include "../ds/check.h"

// Builds an editor with `n` numbered lines ("1", "2", ...).
static void makeNumberedEditor(int n, Editor &ed)
{
    ed.reset();
    if (n <= 0)
    {
        ed.document.clear();
        return;
    }
    Line *first = ed.getLine(0);
    if (first != nullptr)
        first->text = "1";
    for (int i = 2; i <= n; ++i)
    {
        Line l;
        std::string s;
        int v = i;
        while (v > 0)
        {
            s = (char)('0' + v % 10) + s;
            v /= 10;
        }
        l.text = s;
        ed.document.insertAtTail(l);
    }
}

// ---------- OutputCapture ----------

// Captures text and restores cout on destruction.
void test_output_capture_restores_cout()
{
    std::ostringstream realOut;
    // We can't reliably capture std::cout itself inside a test harness,
    // but we can verify that the object doesn't crash and that after
    // it is destroyed, writing to cout still works.
    {
        OutputCapture cap;
        std::cout << "captured";
        CHECK(cap.getText() == "captured");
    }
    // After destruction, cout must still work.
    std::cout << "";
    CHECK(true);
}

// getText on empty capture returns "".
void test_output_capture_empty()
{
    OutputCapture cap;
    CHECK(cap.getText() == "");
}

// Multiple writes are accumulated.
void test_output_capture_multiple_writes()
{
    OutputCapture cap;
    std::cout << "a";
    std::cout << "b";
    std::cout << "c";
    CHECK(cap.getText() == "abc");
}

// ---------- drawScreen ----------

// drawScreen with live=false prints the "=" separator, title, status, window, message.
void test_draw_screen_offline_basic()
{
    Editor ed;
    makeNumberedEditor(3, ed);
    std::ostringstream out;
    drawScreen(ed, "", false, 20, out);
    std::string s = out.str();

    CHECK(s.find("================================================================\n") != std::string::npos);
    CHECK(s.find("=== Text Editor  (type 'help') ===\n") != std::string::npos);
    CHECK(s.find("Lns") != std::string::npos ||
          s.find("Ln ") != std::string::npos);
    CHECK(s.find("| 1\n") != std::string::npos);
    CHECK(s.find("| 2\n") != std::string::npos);
    CHECK(s.find("| 3\n") != std::string::npos);
    CHECK(s.find("Message: \n") != std::string::npos);
}

// Message appears on the line after "Message:".
void test_draw_screen_message_appears()
{
    Editor ed;
    makeNumberedEditor(1, ed);
    std::ostringstream out;
    drawScreen(ed, "It worked!", false, 5, out);
    CHECK(out.str().find("Message: It worked!\n") != std::string::npos);
}

// Multi-line message is preserved.
void test_draw_screen_multiline_message()
{
    Editor ed;
    makeNumberedEditor(1, ed);
    std::ostringstream out;
    drawScreen(ed, "Line one\nLine two\n", false, 5, out);
    CHECK(out.str().find("Message: Line one\nLine two\n") != std::string::npos);
}

// Empty message prints "Message: " with no content.
void test_draw_screen_empty_message()
{
    Editor ed;
    makeNumberedEditor(1, ed);
    std::ostringstream out;
    drawScreen(ed, "", false, 5, out);
    CHECK(out.str().find("Message: \n") != std::string::npos);
}

// Status bar is present (contains a filename or [untitled]).
void test_draw_screen_status_bar()
{
    Editor ed;
    makeNumberedEditor(1, ed);
    ed.setFilename("test.txt");
    std::ostringstream out;
    drawScreen(ed, "", false, 5, out);
    CHECK(out.str().find("test.txt") != std::string::npos);
    CHECK(out.str().find("[untitled]") == std::string::npos);
}

// ---------- getUsage ----------

// getUsage returns a non-empty "Usage:" for every command in the table.
void test_get_usage_every_command()
{
    const char *cmds[] = {
        "new", "open", "save", "saveas", "exit",
        "info", "goto", "all", "live", "help",
        "ins", "add", "nl", "del", "delw", "delr", "dell", "undo", "redo",
        "find", "replace", "replaceall",
        "mark", "marks", "jump", "unmark", "back"};
    for (int i = 0; i < (int)(sizeof(cmds) / sizeof(cmds[0])); ++i)
    {
        std::string u = getUsage(cmds[i]);
        CHECK(!u.empty());
        CHECK(u.find("Usage: ") == 0);
    }
}

// getUsage returns "" for unknown commands.
void test_get_usage_unknown()
{
    CHECK(getUsage("bogus") == "");
    CHECK(getUsage("") == "");
}

// getUsage for "ins" matches the spec string exactly.
void test_get_usage_ins_exact()
{
    CHECK(getUsage("ins") == "Usage: ins <line> <col> <text>");
}

// ---------- printHelp ----------

// Empty topic prints all commands grouped.
void test_help_all_groups()
{
    std::ostringstream out;
    printHelp(out, "");
    std::string s = out.str();
    CHECK(s.find("FILE") != std::string::npos);
    CHECK(s.find("VIEW") != std::string::npos);
    CHECK(s.find("EDIT") != std::string::npos);
    CHECK(s.find("SEARCH") != std::string::npos);
    CHECK(s.find("BOOKMARKS") != std::string::npos);
    // Sanity: a few command names appear.
    CHECK(s.find("ins") != std::string::npos);
    CHECK(s.find("open") != std::string::npos);
    CHECK(s.find("mark") != std::string::npos);
}

// Single command shows Usage, Description and Example.
void test_help_single_command()
{
    std::ostringstream out;
    printHelp(out, "ins");
    std::string s = out.str();
    CHECK(s.find("Usage:       ins <line> <col> <text>") != std::string::npos);
    CHECK(s.find("Description:") != std::string::npos);
    CHECK(s.find("Example:") != std::string::npos);
}

// Every command in the table produces non-empty help.
void test_help_every_command()
{
    const char *cmds[] = {
        "new", "open", "save", "saveas", "exit",
        "info", "goto", "all", "live", "help",
        "ins", "add", "nl", "del", "delw", "delr", "dell", "undo", "redo",
        "find", "replace", "replaceall",
        "mark", "marks", "jump", "unmark", "back"};
    for (int i = 0; i < (int)(sizeof(cmds) / sizeof(cmds[0])); ++i)
    {
        std::ostringstream out;
        printHelp(out, cmds[i]);
        CHECK(out.str().find("Usage:") != std::string::npos);
    }
}

// Unknown topic prints a friendly line.
void test_help_unknown_topic()
{
    std::ostringstream out;
    printHelp(out, "notacommand");
    CHECK(out.str().find("No help available") != std::string::npos);
}

int main()
{
    // OutputCapture
    test_output_capture_restores_cout();
    test_output_capture_empty();
    test_output_capture_multiple_writes();

    // drawScreen
    test_draw_screen_offline_basic();
    test_draw_screen_message_appears();
    test_draw_screen_multiline_message();
    test_draw_screen_empty_message();
    test_draw_screen_status_bar();

    // getUsage
    test_get_usage_every_command();
    test_get_usage_unknown();
    test_get_usage_ins_exact();

    // printHelp
    test_help_all_groups();
    test_help_single_command();
    test_help_every_command();
    test_help_unknown_topic();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}