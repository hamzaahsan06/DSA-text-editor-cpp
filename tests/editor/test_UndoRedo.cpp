// g++ -std=c++17 -Wall -Wextra -Iinclude -Iinclude/editor tests/editor/test_UndoRedo.cpp src/editor/UndoRedo.cpp -o build/test_UndoRedo ; if ($?) { ./build/test_UndoRedo }

#include <iostream>
#include <string>
#include "editor/UndoRedo.h"
#include "editor/Editor.h"
#include "editor/Types.h"
#include "../ds/check.h"

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

// Checks record() rejects a line index that does not exist.
void test_record_rejects_missing_line()
{
    Editor ed; makeEditor(ed);
    UndoRedo ur;
    Command c;
    c.type = INSERT_TEXT;
    c.line = 99;
    c.col = 0;
    c.newText = "x";
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
    CHECK(ur.recordReplace(ed, 0, 5, "old", "new") == true);
    Command c;
    CHECK(ed.undoStack.peek(c) == true);
    CHECK(c.type == REPLACE);
    CHECK(c.line == 0);
    CHECK(c.col == 5);
    CHECK(c.oldText == "old");
    CHECK(c.newText == "new");
}

// ---------- recordInsertLine / recordDeleteLine ----------

// Checks recordInsertLine accepts line == getLineCount() (append at end).
void test_record_insert_line_at_end()
{
    Editor ed; makeEditor(ed);
    UndoRedo ur;
    int end = ed.getLineCount();
    CHECK(ur.recordInsertLine(ed, end, "appended") == true);
    CHECK(ed.undoStack.size() == 1);
    Command c;
    CHECK(ed.undoStack.peek(c) == true);
    CHECK(c.type == INSERT_LINE);
    CHECK(c.line == end);
    CHECK(c.newText == "appended");
}

// Checks recordInsertLine rejects negative or out-of-range line.
void test_record_insert_line_rejects_bad_index()
{
    Editor ed; makeEditor(ed);
    UndoRedo ur;
    CHECK(ur.recordInsertLine(ed, -1, "x") == false);
    CHECK(ur.recordInsertLine(ed, ed.getLineCount() + 1, "x") == false);
    CHECK(ed.undoStack.isEmpty());
}

// Checks recordDeleteLine accepts a valid line and rejects bad indexes.
void test_record_delete_line_valid_and_bad()
{
    Editor ed; makeEditor(ed);
    Line a; a.text = "alpha";
    ed.document.insertAtTail(a);   // now ["", "alpha"], 2 lines
    UndoRedo ur;
    CHECK(ur.recordDeleteLine(ed, 0, "") == true);         // line 0 valid
    CHECK(ur.recordDeleteLine(ed, 1, "alpha") == true);    // line 1 valid
    CHECK(ed.undoStack.size() == 2);
    CHECK(ur.recordDeleteLine(ed, -1, "x") == false);
    CHECK(ur.recordDeleteLine(ed, 2, "x") == false);       // out of range (count 2)
    CHECK(ed.undoStack.size() == 2);
}

// ---------- undo of text ops ----------

// Checks undo on empty history returns false.
void test_undo_empty_history()
{
    Editor ed; makeEditor(ed);
    UndoRedo ur;
    CHECK(ur.undo(ed) == false);
    CHECK(ed.undoStack.isEmpty());
    CHECK(ed.redoStack.isEmpty());
}

// Checks undo of INSERT_TEXT removes the inserted text.
void test_undo_insert_text()
{
    Editor ed; makeEditor(ed);
    Line* l = ed.getLine(0);
    l->text = "hello";                         // simulate the edit
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
    l->text = "abc";                           // simulate "abcde" -> erased "de"
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
    l->text = "new";                           // simulate "old" -> "new"
    UndoRedo ur;
    ur.recordReplace(ed, 0, 0, "old", "new");
    CHECK(ur.undo(ed) == true);
    CHECK(l->text == "old");
}

// Checks undo refuses out-of-range column and preserves history.
void test_undo_out_of_range_col()
{
    Editor ed; makeEditor(ed);
    Line* l = ed.getLine(0);
    l->text = "abc";
    UndoRedo ur;
    ur.recordInsert(ed, 0, 2, "xy");           // would erase 2 chars at col 2, but only 1 there
    CHECK(ur.undo(ed) == false);
    CHECK(l->text == "abc");
    CHECK(ed.undoStack.size() == 1);
}

// Checks multi-line undo targets the correct line.
void test_undo_multi_line()
{
    Editor ed; makeEditor(ed);
    Line* first = ed.getLine(0);
    first->text = "first";
    Line l2; l2.text = "second";
    Line l3; l3.text = "third";
    ed.document.insertAtTail(l2);
    ed.document.insertAtTail(l3);              // ["first", "second", "third"]

    UndoRedo ur;
    ur.recordReplace(ed, 2, 0, "third", "THIRD");
    Line* target = ed.getLine(2);
    target->text = "THIRD";                    // simulate the edit

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

// ---------- undo of line ops ----------

// Checks undo of INSERT_LINE removes that line.
void test_undo_insert_line()
{
    Editor ed; makeEditor(ed);
    UndoRedo ur;
    ur.recordInsertLine(ed, 1, "added");       // record BEFORE mutating
    Line extra; extra.text = "added";
    ed.document.insertAtTail(extra);           // then apply -> ["", "added"]

    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLineCount() == 1);
    CHECK(ed.getLine(1) == nullptr);
    CHECK(ed.getLine(0)->text == "");
}

// Checks undo of DELETE_LINE restores a single removed line.
void test_undo_delete_line_single()
{
    Editor ed; makeEditor(ed);
    Line a; a.text = "alpha";
    Line b; b.text = "beta";
    ed.document.insertAtTail(a);
    ed.document.insertAtTail(b);               // ["", "alpha", "beta"]

    UndoRedo ur;
    ur.recordDeleteLine(ed, 1, "alpha");       // record FIRST (line 1 exists)
    Line removed;
    ed.document.removeAtPosition(1, removed);  // then mutate -> ["", "beta"]

    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLineCount() == 3);
    CHECK(ed.getLine(1)->text == "alpha");
    CHECK(ed.getLine(2)->text == "beta");
}

// Checks undo of DELETE_LINE restores a multi-line blob.
void test_undo_delete_line_multi_blob()
{
    Editor ed; makeEditor(ed);
    Line a; a.text = "one";
    Line b; b.text = "two";
    Line c; c.text = "three";
    ed.document.insertAtTail(a);
    ed.document.insertAtTail(b);
    ed.document.insertAtTail(c);               // ["", "one", "two", "three"]

    UndoRedo ur;
    ur.recordDeleteLine(ed, 1, "one\ntwo");    // record FIRST

    Line tmp;
    ed.document.removeAtPosition(1, tmp);
    ed.document.removeAtPosition(1, tmp);      // then mutate -> ["", "three"]

    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLineCount() == 4);
    CHECK(ed.getLine(0)->text == "");
    CHECK(ed.getLine(1)->text == "one");
    CHECK(ed.getLine(2)->text == "two");
    CHECK(ed.getLine(3)->text == "three");
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

// Checks undo then redo of INSERT_LINE round-trips.
void test_redo_insert_line()
{
    Editor ed; makeEditor(ed);
    UndoRedo ur;
    ur.recordInsertLine(ed, 1, "extra");       // record FIRST
    Line extra; extra.text = "extra";
    ed.document.insertAtTail(extra);           // then apply

    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLineCount() == 1);
    CHECK(ur.redo(ed) == true);
    CHECK(ed.getLineCount() == 2);
    CHECK(ed.getLine(1)->text == "extra");
}

// Checks undo then redo of a single-line DELETE_LINE round-trips.
void test_redo_delete_line_single()
{
    Editor ed; makeEditor(ed);
    Line a; a.text = "alpha";
    ed.document.insertAtTail(a);               // ["", "alpha"]

    UndoRedo ur;
    ur.recordDeleteLine(ed, 1, "alpha");       // record FIRST (line 1 exists)
    Line removed;
    ed.document.removeAtPosition(1, removed);  // then mutate -> [""]

    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLineCount() == 2);
    CHECK(ed.getLine(1)->text == "alpha");
    CHECK(ur.redo(ed) == true);
    CHECK(ed.getLineCount() == 1);
    CHECK(ed.getLine(1) == nullptr);
}

// Checks undo then redo of a multi-line DELETE_LINE round-trips.
void test_redo_delete_line_multi()
{
    Editor ed; makeEditor(ed);
    Line a; a.text = "one";
    Line b; b.text = "two";
    Line c; c.text = "three";
    ed.document.insertAtTail(a);
    ed.document.insertAtTail(b);
    ed.document.insertAtTail(c);               // ["", "one", "two", "three"]

    UndoRedo ur;
    ur.recordDeleteLine(ed, 1, "one\ntwo");    // record FIRST

    Line tmp;
    ed.document.removeAtPosition(1, tmp);
    ed.document.removeAtPosition(1, tmp);      // then mutate -> ["", "three"]

    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLineCount() == 4);
    CHECK(ed.getLine(1)->text == "one");
    CHECK(ed.getLine(2)->text == "two");

    CHECK(ur.redo(ed) == true);
    CHECK(ed.getLineCount() == 2);
    CHECK(ed.getLine(1)->text == "three");
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
    l->text = "x";                             // simulate the edit
    CHECK(ur.canUndo(ed) == true);
    CHECK(ur.canRedo(ed) == false);

    CHECK(ur.undo(ed) == true);
    CHECK(ur.canUndo(ed) == false);
    CHECK(ur.canRedo(ed) == true);

    CHECK(ur.redo(ed) == true);
    CHECK(ur.canUndo(ed) == true);
    CHECK(ur.canRedo(ed) == false);
}

// Checks clearHistory empties both stacks.
void test_clear_history()
{
    Editor ed; makeEditor(ed);
    UndoRedo ur;
    Line* l = ed.getLine(0);

    ur.recordInsert(ed, 0, 0, "a");
    l->text = "a";
    CHECK(ur.undo(ed) == true);

    ur.recordInsert(ed, 0, 0, "b");
    l->text = "b";
    CHECK(ur.undo(ed) == true);

    CHECK(ed.undoStack.isEmpty() == true);
    CHECK(ed.redoStack.isEmpty() == false);

    ur.clearHistory(ed);
    CHECK(ed.undoStack.isEmpty());
    CHECK(ed.redoStack.isEmpty());
}

// ---------- FORMAT ops are refused ----------

// Checks FORMAT_APPLY and FORMAT_REMOVE are refused (stacks untouched).
void test_format_ops_refused()
{
    Editor ed; makeEditor(ed);
    UndoRedo ur;

    Command fmtApply;
    fmtApply.type = FORMAT_APPLY;
    fmtApply.line = 0;
    fmtApply.col = 0;
    fmtApply.fmtStart = 0;
    fmtApply.fmtEnd = 1;
    fmtApply.fmtType = BOLD;
    CHECK(ur.record(ed, fmtApply) == true);    // record accepts it
    CHECK(ur.undo(ed) == false);               // but undo refuses
    CHECK(ed.undoStack.size() == 1);           // command stays

    Command fmtRemove;
    fmtRemove.type = FORMAT_REMOVE;
    fmtRemove.line = 0;
    fmtRemove.col = 0;
    fmtRemove.fmtStart = 0;
    fmtRemove.fmtEnd = 1;
    fmtRemove.fmtType = ITALIC;
    CHECK(ur.record(ed, fmtRemove) == true);
    CHECK(ur.undo(ed) == false);
    CHECK(ed.undoStack.size() == 2);
}

int main()
{
    test_record_valid_command();
    test_record_rejects_bad_position();
    test_record_rejects_missing_line();
    test_record_clears_redo_stack();
    test_record_insert_builds_command();
    test_record_erase_builds_command();
    test_record_replace_builds_command();

    test_record_insert_line_at_end();
    test_record_insert_line_rejects_bad_index();
    test_record_delete_line_valid_and_bad();

    test_undo_empty_history();
    test_undo_insert_text();
    test_undo_delete_text();
    test_undo_replace();
    test_undo_out_of_range_col();
    test_undo_multi_line();
    test_undo_sets_modified();
    test_undo_insert_line();
    test_undo_delete_line_single();
    test_undo_delete_line_multi_blob();

    test_redo_empty_history();
    test_redo_insert_after_undo();
    test_redo_delete_after_undo();
    test_redo_replace_after_undo();
    test_redo_insert_line();
    test_redo_delete_line_single();
    test_redo_delete_line_multi();
    test_redo_sets_modified();

    test_can_undo_can_redo();
    test_clear_history();
    test_format_ops_refused();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}