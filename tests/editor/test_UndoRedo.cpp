//g++ -std=c++17 -Wall -Wextra -Iinclude tests/editor/test_UndoRedo.cpp src/editor/UndoRedo.cpp -o test_undoredo.exe

//.\test_undoredo.exe

#include <iostream>
#include <string>
#include "editor/UndoRedo.h"
#include "editor/Editor.h"

static int passed = 0, failed = 0;

#define CHECK(cond) \
    do \
    { \
        if (cond) \
            ++passed; \
        else \
        { \
            ++failed; \
            std::cout << "FAIL line " << __LINE__ << ": " #cond "\n"; \
        } \
    } while (0)

// Expected: A newly created Editor has empty undo and redo stacks.
void test_initial_state_empty()
{
    Editor ed;
    UndoRedo ur;

    CHECK(ur.canUndo(ed) == false);
    CHECK(ur.canRedo(ed) == false);
}

// Expected: Recording an insert operation adds it to undo history.
void test_record_insert()
{
    Editor ed;
    UndoRedo ur;

    CHECK(ur.recordInsert(ed, 0, "Hello") == true);
    CHECK(ur.canUndo(ed) == true);
    CHECK(ur.canRedo(ed) == false);
}

// Expected: Recording an erase operation adds it to undo history.
void test_record_erase()
{
    Editor ed;
    UndoRedo ur;

    CHECK(ur.recordErase(ed, 0, "Hello") == true);
    CHECK(ur.canUndo(ed) == true);
}

// Expected: Recording a replace operation adds it to undo history.
void test_record_replace()
{
    Editor ed;
    UndoRedo ur;

    CHECK(ur.recordReplace(ed, 0, "Hello", "World") == true);
    CHECK(ur.canUndo(ed) == true);
}

// Expected: Recording an operation with a negative position is rejected.
void test_invalid_negative_position()
{
    Editor ed;
    UndoRedo ur;

    CHECK(ur.recordInsert(ed, -1, "Hello") == false);
    CHECK(ur.canUndo(ed) == false);
}

// Expected: Undo reverses an insertion operation.
void test_undo_insert()
{
    Editor ed;
    UndoRedo ur;

    Line* line = ed.getLine(0);
    line->text = "Hello!";

    CHECK(ur.recordInsert(ed, 5, "!") == true);

    CHECK(ur.undo(ed) == true);
    CHECK(line->text == "Hello");
    CHECK(ur.canRedo(ed) == true);
    CHECK(ur.canUndo(ed) == false);
}

// Expected: Redo restores an insertion operation.
void test_redo_insert()
{
    Editor ed;
    UndoRedo ur;

    Line* line = ed.getLine(0);
    line->text = "Hello!";

    ur.recordInsert(ed, 5, "!");

    CHECK(ur.undo(ed) == true);
    CHECK(line->text == "Hello");

    CHECK(ur.redo(ed) == true);
    CHECK(line->text == "Hello!");
    CHECK(ur.canUndo(ed) == true);
    CHECK(ur.canRedo(ed) == false);
}

// Expected: Undo reverses a deletion operation.
void test_undo_erase()
{
    Editor ed;
    UndoRedo ur;

    Line* line = ed.getLine(0);
    line->text = "Hello";

    CHECK(ur.recordErase(ed, 5, " World") == true);

    line->text.erase(5, 6);

    CHECK(line->text == "Hello");

    CHECK(ur.undo(ed) == true);
    CHECK(line->text == "Hello World");
}

// Expected: Redo restores a deletion operation.
void test_redo_erase()
{
    Editor ed;
    UndoRedo ur;

    Line* line = ed.getLine(0);
    line->text = "Hello";

    ur.recordErase(ed, 5, " World");

    line->text.erase(5, 6);

    CHECK(ur.undo(ed) == true);
    CHECK(line->text == "Hello World");

    CHECK(ur.redo(ed) == true);
    CHECK(line->text == "Hello");
}

// Expected: Undo reverses a replacement operation.
void test_undo_replace()
{
    Editor ed;
    UndoRedo ur;

    Line* line = ed.getLine(0);
    line->text = "World";

    CHECK(ur.recordReplace(ed, 0, "Hello", "World") == true);

    CHECK(ur.undo(ed) == true);
    CHECK(line->text == "Hello");
}

// Expected: Redo restores a replacement operation.
void test_redo_replace()
{
    Editor ed;
    UndoRedo ur;

    Line* line = ed.getLine(0);
    line->text = "World";

    ur.recordReplace(ed, 0, "Hello", "World");

    CHECK(ur.undo(ed) == true);
    CHECK(line->text == "Hello");

    CHECK(ur.redo(ed) == true);
    CHECK(line->text == "World");
}

// Expected: Undo fails when undo history is empty.
void test_undo_empty_history()
{
    Editor ed;
    UndoRedo ur;

    CHECK(ur.undo(ed) == false);
}

// Expected: Redo fails when redo history is empty.
void test_redo_empty_history()
{
    Editor ed;
    UndoRedo ur;

    CHECK(ur.redo(ed) == false);
}

// Expected: Recording a new operation clears redo history.
void test_new_operation_clears_redo()
{
    Editor ed;
    UndoRedo ur;

    Line* line = ed.getLine(0);
    line->text = "Hello!";

    ur.recordInsert(ed, 5, "!");

    CHECK(ur.undo(ed) == true);
    CHECK(ur.canRedo(ed) == true);

    CHECK(ur.recordInsert(ed, 5, " World") == true);

    CHECK(ur.canRedo(ed) == false);
}

// Expected: Clearing history removes all undo and redo operations.
void test_clear_history()
{
    Editor ed;
    UndoRedo ur;

    ur.recordInsert(ed, 0, "Hello");
    ur.recordErase(ed, 5, " World");

    CHECK(ur.canUndo(ed) == true);

    ur.clearHistory(ed);

    CHECK(ur.canUndo(ed) == false);
    CHECK(ur.canRedo(ed) == false);
}

// Expected: Multiple undo operations work in reverse order.
void test_multiple_undo()
{
    Editor ed;
    UndoRedo ur;

    Line* line = ed.getLine(0);
    line->text = "ABC";

    ur.recordInsert(ed, 2, "C");
    ur.recordInsert(ed, 1, "B");

    CHECK(ur.undo(ed) == true);
    CHECK(line->text == "AB");

    CHECK(ur.undo(ed) == true);
    CHECK(line->text == "A");

    CHECK(ur.canUndo(ed) == false);
}

// Expected: Multiple redo operations restore operations in correct order.
void test_multiple_redo()
{
    Editor ed;
    UndoRedo ur;

    Line* line = ed.getLine(0);
    line->text = "ABC";

    ur.recordInsert(ed, 2, "C");
    ur.recordInsert(ed, 1, "B");

    ur.undo(ed);
    ur.undo(ed);

    CHECK(line->text == "A");

    CHECK(ur.redo(ed) == true);
    CHECK(line->text == "AB");

    CHECK(ur.redo(ed) == true);
    CHECK(line->text == "ABC");
}

int main()
{
    test_initial_state_empty();
    test_record_insert();
    test_record_erase();
    test_record_replace();
    test_invalid_negative_position();
    test_undo_insert();
    test_redo_insert();
    test_undo_erase();
    test_redo_erase();
    test_undo_replace();
    test_redo_replace();
    test_undo_empty_history();
    test_redo_empty_history();
    test_new_operation_clears_redo();
    test_clear_history();
    test_multiple_undo();
    test_multiple_redo();

    std::cout << passed << " passed, " << failed << " failed\n";

    return failed == 0 ? 0 : 1;
}