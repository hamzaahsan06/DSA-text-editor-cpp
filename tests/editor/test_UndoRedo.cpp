// g++ -std=c++17 -Wall -Wextra -Iinclude -Iinclude/editor tests/editor/test_UndoRedo.cpp src/editor/UndoRedo.cpp -o build/test_UndoRedo ; if ($?) { ./build/test_UndoRedo }

#include <iostream>
#include <string>
#include "editor/UndoRedo.h"
#include "editor/Editor.h"
#include "editor/Types.h"

static int passed = 0, failed = 0;
#define CHECK(cond) do { if (cond) ++passed; else { ++failed; \
    std::cout << "FAIL line " << __LINE__ << ": " #cond "\n"; } } while (0)

// Resets `ed` to a fresh editor with one empty line.
// Editor is non-copyable, so we build it in place via a reference out-parameter.
static void makeEditor(Editor& ed)
{
    ed.reset();
}

// ---------- record / recordInsert / recordErase / recordReplace ----------

// Checks record() accepts a valid command and puts it on undoStack.
void test_record_valid_command()
{
    Editor ed; makeEditor(ed);
    UndoRedo ur;
    Command c;
    c.type = INSERT_TEXT;
    c.line = 0;
    c.col = 0;
    c.newText = "hi";
    CHECK(ur.record(ed, c) == true);
    CHECK(ed.undoStack.size() == 1);
    CHECK(ed.redoStack.isEmpty());
}

// Checks record() rejects negative line or column.
void test_record_rejects_bad_position()
{
    Editor ed; makeEditor(ed);
    UndoRedo ur;
    Command c;
    c.type = INSERT_TEXT;
    c.line = -1;
    c.col = 0;
    CHECK(ur.record(ed, c) == false);
    c.line = 0;
    c.col = -1;
    CHECK(ur.record(ed, c) == false);
    CHECK(ed.undoStack.isEmpty());
}

// Checks record() clears redoStack on a new command.
void test_record_clears_redo_stack()
{
    Editor ed; makeEditor(ed);
    UndoRedo ur;
    Command c;
    c.type = INSERT_TEXT;
    c.line = 0;
    c.col = 0;
    c.newText = "a";
    ur.record(ed, c);
    ed.redoStack.push(c);
    CHECK(ed.redoStack.size() == 1);
    ur.record(ed, c);
    CHECK(ed.redoStack.isEmpty());
    CHECK(ed.undoStack.size() == 2);
}

// Checks recordInsert stores line, col and text correctly.
void test_record_insert_builds_command()
{
    Editor ed; makeEditor(ed);
    UndoRedo ur;
    CHECK(ur.recordInsert(ed, 0, 3, "abc") == true);
    Command c;
    CHECK(ed.undoStack.peek(c) == true);
    CHECK(c.type == INSERT_TEXT);
    CHECK(c.line == 0);
    CHECK(c.col == 3);
    CHECK(c.oldText == "");
    CHECK(c.newText == "abc");
}

// Checks recordErase stores erasedText in oldText and empty newText.
void test_record_erase_builds_command()
{
    Editor ed; makeEditor(ed);
    UndoRedo ur;
    CHECK(ur.recordErase(ed, 0, 2, "xy") == true);
    Command c;
    CHECK(ed.undoStack.peek(c) == true);
    CHECK(c.type == DELETE_TEXT);
    CHECK(c.line == 0);
    CHECK(c.col == 2);
    CHECK(c.oldText == "xy");
    CHECK(c.newText == "");
}

// Checks recordReplace stores old and new text with line/col.
void test_record_replace_builds_command()
{
    Editor ed; makeEditor(ed);
    UndoRedo ur;
    CHECK(ur.recordReplace(ed, 2, 5, "old", "new") == true);
    Command c;
    CHECK(ed.undoStack.peek(c) == true);
    CHECK(c.type == REPLACE);
    CHECK(c.line == 2);
    CHECK(c.col == 5);
    CHECK(c.oldText == "old");
    CHECK(c.newText == "new");
}

// ---------- undo ----------

// Checks undo on empty history returns false and does not modify the doc.
void test_undo_empty_history()
{
    Editor ed; makeEditor(ed);
    UndoRedo ur;
    CHECK(ur.undo(ed) == false);
    Line* l = ed.getLine(0);
    CHECK(l != nullptr);
    CHECK(l->text == "");
    CHECK(ed.undoStack.isEmpty());
    CHECK(ed.redoStack.isEmpty());
}

// Checks undo of INSERT_TEXT removes the inserted text.
void test_undo_insert_text()
{
    Editor ed; makeEditor(ed);
    Line* l = ed.getLine(0);
    l->text = "hello";                 // simulate the insert having happened
    UndoRedo ur;
    ur.recordInsert(ed, 0, 0, "hello");
    CHECK(ur.undo(ed) == true);
    CHECK(l->text == "");
    CHECK(ed.undoStack.isEmpty());
    CHECK(ed.redoStack.size() == 1);
}

// Checks undo of DELETE_TEXT re-inserts the erased text.
void test_undo_delete_text()
{
    Editor ed; makeEditor(ed);
    Line* l = ed.getLine(0);
    l->text = "abc";                   // simulate "abcde" -> erased "de" -> "abc"
    UndoRedo ur;
    ur.recordErase(ed, 0, 3, "de");
    CHECK(ur.undo(ed) == true);
    CHECK(l->text == "abcde");
    CHECK(ed.undoStack.isEmpty());
    CHECK(ed.redoStack.size() == 1);
}

// Checks undo of REPLACE restores oldText.
void test_undo_replace()
{
    Editor ed; makeEditor(ed);
    Line* l = ed.getLine(0);
    l->text = "new";                   // simulate "old" -> "new"
    UndoRedo ur;
    ur.recordReplace(ed, 0, 0, "old", "new");
    CHECK(ur.undo(ed) == true);
    CHECK(l->text == "old");
}

// Checks undo on a non-existent line returns false without corrupting stacks.
void test_undo_bad_line()
{
    Editor ed; makeEditor(ed);
    UndoRedo ur;
    Command c;
    c.type = INSERT_TEXT;
    c.line = 99;
    c.col = 0;
    c.newText = "x";
    ur.record(ed, c);
    CHECK(ur.undo(ed) == false);
    CHECK(ed.undoStack.size() == 1);   // command must remain because undo refused
}

// Checks undo refuses out-of-range column and preserves history.
void test_undo_out_of_range_col()
{
    Editor ed; makeEditor(ed);
    Line* l = ed.getLine(0);
    l->text = "abc";
    UndoRedo ur;
    // INSERT_TEXT "xy" at col 2 would erase 2 chars at index 2,
    // but "abc" only has 1 char there -> out of range, must refuse.
    ur.recordInsert(ed, 0, 2, "xy");
    CHECK(ur.undo(ed) == false);
    CHECK(l->text == "abc");
    CHECK(ed.undoStack.size() == 1);
}

// Checks multi-line undo targets the correct line.
void test_undo_multi_line()
{
    Editor ed; makeEditor(ed);
    // reset() leaves line 0 blank; overwrite it, then append two more.
    Line* first = ed.getLine(0);
    first->text = "first";
    Line l2; l2.text = "second";
    Line l3; l3.text = "third";
    ed.document.insertAtTail(l2);
    ed.document.insertAtTail(l3);

    UndoRedo ur;
    ur.recordReplace(ed, 2, 0, "third", "THIRD");
    Line* target = ed.getLine(2);
    target->text = "THIRD";            // simulate the replace having happened

    CHECK(ur.undo(ed) == true);
    Line* l = ed.getLine(2);
    CHECK(l != nullptr);
    CHECK(l->text == "third");
    CHECK(ed.getLine(0)->text == "first");
    CHECK(ed.getLine(1)->text == "second");
}

// Checks undo sets the modified flag.
void test_undo_sets_modified()
{
    Editor ed; makeEditor(ed);
    Line* l = ed.getLine(0);
    l->text = "x";
    ed.setModified(false);
    UndoRedo ur;
    ur.recordInsert(ed, 0, 0, "x");
    CHECK(ur.undo(ed) == true);
    CHECK(ed.isModified() == true);
}

// ---------- redo ----------

// Checks redo on empty history returns false.
void test_redo_empty_history()
{
    Editor ed; makeEditor(ed);
    UndoRedo ur;
    CHECK(ur.redo(ed) == false);
}

// Checks undo then redo of INSERT_TEXT round-trips.
void test_redo_insert_after_undo()
{
    Editor ed; makeEditor(ed);
    Line* l = ed.getLine(0);
    l->text = "world";
    UndoRedo ur;
    ur.recordInsert(ed, 0, 0, "world");
    CHECK(ur.undo(ed) == true);
    CHECK(l->text == "");
    CHECK(ur.redo(ed) == true);
    CHECK(l->text == "world");
    CHECK(ed.undoStack.size() == 1);
    CHECK(ed.redoStack.isEmpty());
}

// Checks undo then redo of DELETE_TEXT round-trips.
void test_redo_delete_after_undo()
{
    Editor ed; makeEditor(ed);
    Line* l = ed.getLine(0);
    l->text = "abc";
    UndoRedo ur;
    ur.recordErase(ed, 0, 3, "de");
    CHECK(ur.undo(ed) == true);
    CHECK(l->text == "abcde");
    CHECK(ur.redo(ed) == true);
    CHECK(l->text == "abc");
}

// Checks undo then redo of REPLACE round-trips.
void test_redo_replace_after_undo()
{
    Editor ed; makeEditor(ed);
    Line* l = ed.getLine(0);
    l->text = "new";
    UndoRedo ur;
    ur.recordReplace(ed, 0, 0, "old", "new");
    CHECK(ur.undo(ed) == true);
    CHECK(l->text == "old");
    CHECK(ur.redo(ed) == true);
    CHECK(l->text == "new");
}

// Checks redo sets the modified flag.
void test_redo_sets_modified()
{
    Editor ed; makeEditor(ed);
    Line* l = ed.getLine(0);
    l->text = "x";
    UndoRedo ur;
    ur.recordInsert(ed, 0, 0, "x");
    ur.undo(ed);
    ed.setModified(false);
    CHECK(ur.redo(ed) == true);
    CHECK(ed.isModified() == true);
}

// ---------- canUndo / canRedo / clearHistory ----------

// Checks canUndo / canRedo reflect stack state after a real insert.
void test_can_undo_can_redo()
{
    Editor ed; makeEditor(ed);
    UndoRedo ur;
    CHECK(ur.canUndo(ed) == false);
    CHECK(ur.canRedo(ed) == false);

    ur.recordInsert(ed, 0, 0, "x");
    Line* l = ed.getLine(0);
    l->text = "x";                     // simulate the insert having happened
    CHECK(ur.canUndo(ed) == true);
    CHECK(ur.canRedo(ed) == false);

    CHECK(ur.undo(ed) == true);        // now "x" -> "" succeeds
    CHECK(ur.canUndo(ed) == false);
    CHECK(ur.canRedo(ed) == true);

    CHECK(ur.redo(ed) == true);
    CHECK(ur.canUndo(ed) == true);
    CHECK(ur.canRedo(ed) == false);
}

// Checks clearHistory empties both stacks (after two real undos).
void test_clear_history()
{
    Editor ed; makeEditor(ed);
    UndoRedo ur;
    Line* l = ed.getLine(0);

    ur.recordInsert(ed, 0, 0, "a");
    l->text = "a";
    CHECK(ur.undo(ed) == true);        // "" -> "a" undone -> ""

    ur.recordInsert(ed, 0, 0, "b");
    l->text = "b";
    CHECK(ur.undo(ed) == true);        // "" -> "b" undone -> ""

    CHECK(ed.undoStack.isEmpty() == true);    // both commands undone
    CHECK(ed.redoStack.isEmpty() == false);   // both now in redo

    ur.clearHistory(ed);
    CHECK(ed.undoStack.isEmpty());
    CHECK(ed.redoStack.isEmpty());
}

int main()
{
    test_record_valid_command();
    test_record_rejects_bad_position();
    test_record_clears_redo_stack();
    test_record_insert_builds_command();
    test_record_erase_builds_command();
    test_record_replace_builds_command();
    test_undo_empty_history();
    test_undo_insert_text();
    test_undo_delete_text();
    test_undo_replace();
    test_undo_bad_line();
    test_undo_out_of_range_col();
    test_undo_multi_line();
    test_undo_sets_modified();
    test_redo_empty_history();
    test_redo_insert_after_undo();
    test_redo_delete_after_undo();
    test_redo_replace_after_undo();
    test_redo_sets_modified();
    test_can_undo_can_redo();
    test_clear_history();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}