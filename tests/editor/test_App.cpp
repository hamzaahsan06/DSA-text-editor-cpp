// g++ -std=c++11 -Wall -Wextra -Iinclude -Iinclude/editor tests/editor/test_App.cpp src/editor/EditorApp.cpp src/editor/CommandParser.cpp src/editor/Screen.cpp src/editor/Display.cpp src/editor/FileManager.cpp src/editor/TextOps.cpp src/editor/SearchReplace.cpp src/editor/Bookmarks.cpp src/editor/UndoRedo.cpp src/editor/Formatting.cpp src/editor/Validation.cpp -o build/test_App ; if ($?) { ./build/test_App }

#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include "editor/EditorApp.h"
#include "editor/Editor.h"
#include "editor/Types.h"
#include "../ds/check.h"

// Creates an EditorApp in non-live mode with the given stdin.
static void setupApp(EditorApp &app, std::istringstream &in)
{
    app.setLive(false);
    app.setInput(in);
}

// ---------- constructor / accessors ----------

// Fresh app has one blank line, empty message, running=true, live=true (default).
void test_initial_state()
{
    EditorApp app;
    CHECK(app.isRunning() == true);
    CHECK(app.getMessage() == "");
    CHECK(app.getEditor().getLineCount() == 1);
    CHECK(app.getEditor().getLine(0)->text == "");
}

// ---------- new ----------

// "new" resets to a single blank line.
void test_new_resets()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.getEditor().getLine(0)->text = "abc";
    app.getEditor().setModified(false);
    app.handleLine("new");
    CHECK(app.getEditor().getLineCount() == 1);
    CHECK(app.getEditor().getLine(0)->text == "");
    CHECK(app.getMessage() == "New document.");
}

// "NEW" is case-insensitive.
void test_new_uppercase()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.handleLine("NEW");
    CHECK(app.getMessage() == "New document.");
}

// ---------- info ----------

// "info" fills the message with the stats block.
void test_info()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.handleLine("info");
    CHECK(app.getMessage().find("Document info") != std::string::npos);
    CHECK(app.getMessage().find("Lines") != std::string::npos);
}

// ---------- goto ----------

// "goto 2" moves the cursor to line 2 col 1 (1-based user input).
void test_goto_line_only()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    // Add two more lines so we have 3 total.
    Line l1;
    l1.text = "aaa";
    Line l2;
    l2.text = "bbb";
    app.getEditor().document.insertAtTail(l1);
    app.getEditor().document.insertAtTail(l2);

    app.handleLine("goto 2");
    CHECK(app.getEditor().cursorLine == 1);
    CHECK(app.getEditor().cursorCol == 0);
    CHECK(app.getMessage().find("Cursor at line 2") != std::string::npos);
}

// "goto 2 3" sets both line and col.
void test_goto_line_and_col()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    Line l1;
    l1.text = "aaa";
    Line l2;
    l2.text = "bbb";
    app.getEditor().document.insertAtTail(l1);
    app.getEditor().document.insertAtTail(l2);

    app.handleLine("goto 2 3");
    CHECK(app.getEditor().cursorLine == 1);
    CHECK(app.getEditor().cursorCol == 2);
}

// "goto abc" is rejected with the usage line.
void test_goto_bad_number()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.handleLine("goto abc");
    CHECK(app.getMessage().find("Usage: goto") != std::string::npos);
}

// "goto 0" is rejected (parsePositiveInt refuses 0).
void test_goto_zero_line()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.handleLine("goto 0");
    CHECK(app.getMessage().find("Usage: goto") != std::string::npos);
}

// "goto -3" is rejected.
void test_goto_negative_line()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.handleLine("goto -3");
    CHECK(app.getMessage().find("Usage: goto") != std::string::npos);
}

// "goto" beyond the last line is rejected.
void test_goto_out_of_range()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.handleLine("goto 5");
    CHECK(app.getMessage().find("out of range") != std::string::npos);
}

// ---------- all ----------

// "all" fills the message with the whole document.
void test_all()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.getEditor().getLine(0)->text = "hello";
    app.handleLine("all");
    CHECK(app.getMessage().find("hello") != std::string::npos);
    CHECK(app.getMessage().find("1 |") != std::string::npos);
}

// ---------- live ----------

// "live off" toggles live to false.
void test_live_off()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.handleLine("live off");
    CHECK(app.getMessage() == "Live screen OFF.");
}

// "live on" toggles live to true.
void test_live_on()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.handleLine("live on");
    CHECK(app.getMessage() == "Live screen ON.");
}

// "live" with no argument shows usage.
void test_live_missing_arg()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.handleLine("live");
    CHECK(app.getMessage().find("Usage: live") != std::string::npos);
}

// ---------- help ----------

// "help" with no topic prints all groups.
void test_help_all()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.handleLine("help");
    CHECK(app.getMessage().find("FILE") != std::string::npos);
    CHECK(app.getMessage().find("EDIT") != std::string::npos);
}

// "help ins" shows usage for ins.
void test_help_topic()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.handleLine("help ins");
    CHECK(app.getMessage().find("Usage:       ins") != std::string::npos);
}

// ---------- undo / redo ----------

// "undo" with empty stack returns "Nothing to undo.".
void test_undo_empty()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.handleLine("undo");
    CHECK(app.getMessage() == "Nothing to undo.");
}

// "redo" with empty stack returns "Nothing to redo.".
void test_redo_empty()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.handleLine("redo");
    CHECK(app.getMessage() == "Nothing to redo.");
}

// "undo abc" is rejected.
void test_undo_bad_number()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.handleLine("undo abc");
    CHECK(app.getMessage().find("Usage: undo") != std::string::npos);
}

// "undo 99999999999" (11 digits) is rejected by parsePositiveInt.
void test_undo_huge_number()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.handleLine("undo 99999999999");
    CHECK(app.getMessage().find("Usage: undo") != std::string::npos);
}

// "undo 5" when only 1 undo is available undoes 1.
void test_undo_more_than_available()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    // Push a fake command onto the undo stack via the editor's stack.
    // (We use the undo stack directly, since TextOps isn't in scope here.)
    Command c;
    c.type = INSERT_TEXT;
    c.startLine = 0;
    Line before;
    before.text = "";
    Line after;
    after.text = "x";
    c.before.pushBack(before);
    c.after.pushBack(after);
    app.getEditor().undoStack.push(c);
    app.getEditor().getLine(0)->text = "x";

    app.handleLine("undo 5");
    CHECK(app.getMessage() == "Undid 1 operation(s).");
}

// ---------- unknown / not-implemented ----------

// Unknown command prints "Unknown command".
void test_unknown()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.handleLine("xyzzy");
    CHECK(app.getMessage().find("Unknown command") != std::string::npos);
}

// Known command not in our impl list -> "Not implemented yet.".
void test_not_implemented()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.handleLine("find hello");
    CHECK(app.getMessage().find("Not implemented yet") != std::string::npos);
    CHECK(app.getMessage().find("Usage:") != std::string::npos);
}

// Blank line clears the message.
void test_blank_line()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.getMessage(); // no-op
    app.handleLine("info");
    CHECK(app.getMessage() != "");
    app.handleLine("   ");
    CHECK(app.getMessage() == "");
}

// ---------- save / saveas / open ----------

// "save" with no filename complains about missing name.
void test_save_no_filename()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.handleLine("save");
    CHECK(app.getMessage().find("no file name") != std::string::npos ||
          app.getMessage().find("Error") != std::string::npos ||
          app.getMessage().find("Save") != std::string::npos);
}

// "saveas" with a filename writes the file.
void test_saveas_writes_file()
{
    const std::string path = "test_app_tmp.txt";
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.getEditor().getLine(0)->text = "hello world";
    app.getEditor().setModified(true);
    app.handleLine("saveas " + path);
    CHECK(app.getMessage().find("Saved") != std::string::npos);
    CHECK(app.getEditor().getFilename() == path);
    std::remove(path.c_str());
}

// "open" a missing file keeps the current document.
void test_open_missing_file()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.getEditor().getLine(0)->text = "keep";
    app.getEditor().setModified(false);
    app.handleLine("open does_not_exist_xyz.txt");
    CHECK(app.getEditor().getLine(0)->text == "keep");
    CHECK(app.getMessage().find("cannot open") != std::string::npos ||
          app.getMessage().find("Could not") != std::string::npos);
}

// ---------- unsaved guard on exit ----------

// exit without changes just stops.
void test_exit_unmodified()
{
    EditorApp app;
    std::istringstream in("");
    setupApp(app, in);
    app.handleLine("exit");
    CHECK(app.isRunning() == false);
}

// exit with changes and answer "n" discards and stops.
void test_exit_discard_with_n()
{
    EditorApp app;
    std::istringstream in("n\n");
    setupApp(app, in);
    app.getEditor().setModified(true);
    app.handleLine("exit");
    CHECK(app.isRunning() == false);
}

// exit with changes and answer "c" cancels, loop keeps going.
void test_exit_cancel_with_c()
{
    EditorApp app;
    std::istringstream in("c\n");
    setupApp(app, in);
    app.getEditor().setModified(true);
    app.handleLine("exit");
    CHECK(app.isRunning() == true);
    CHECK(app.getMessage().find("Cancelled") != std::string::npos);
}

// ---------- unsaved guard with junk answer ----------

// Junk-then-n: asks again, then discards on "n".
void test_exit_junk_then_n()
{
    EditorApp app;
    std::istringstream in("x\nn\n");
    setupApp(app, in);
    app.getEditor().setModified(true);
    app.handleLine("exit");
    CHECK(app.isRunning() == false);
}

int main()
{
    test_initial_state();

    test_new_resets();
    test_new_uppercase();

    test_info();
    test_goto_line_only();
    test_goto_line_and_col();
    test_goto_bad_number();
    test_goto_zero_line();
    test_goto_negative_line();
    test_goto_out_of_range();

    test_all();
    test_live_off();
    test_live_on();
    test_live_missing_arg();

    test_help_all();
    test_help_topic();

    test_undo_empty();
    test_redo_empty();
    test_undo_bad_number();
    test_undo_huge_number();
    test_undo_more_than_available();

    test_unknown();
    test_not_implemented();
    test_blank_line();

    test_save_no_filename();
    test_saveas_writes_file();
    test_open_missing_file();

    test_exit_unmodified();
    test_exit_discard_with_n();
    test_exit_cancel_with_c();
    test_exit_junk_then_n();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}