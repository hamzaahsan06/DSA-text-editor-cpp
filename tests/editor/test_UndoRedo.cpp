// g++ -std=c++17 -Wall -Wextra -Iinclude -Iinclude/editor tests/editor/test_UndoRedo.cpp src/editor/UndoRedo.cpp -o build/test_UndoRedo ; if ($?) { ./build/test_UndoRedo }

#include <iostream>
#include <string>
#include "editor/UndoRedo.h"

static int passed = 0, failed = 0;
#define CHECK(cond)                                                   \
    do                                                                \
    {                                                                 \
        if (cond)                                                     \
            ++passed;                                                 \
        else                                                          \
        {                                                             \
            ++failed;                                                 \
            std::cout << "FAIL line " << __LINE__ << ": " #cond "\n"; \
        }                                                             \
    } while (0)

// Expected: A newly created UndoRedo object has no history.
void test_initial_state_empty()
{
    UndoRedo ur;
    CHECK(ur.canUndo() == false);
    CHECK(ur.canRedo() == false);
}

// Expected: Recording an insert makes canUndo true and clears redo history.
void test_record_insert_updates_state()
{
    UndoRedo ur;
    CHECK(ur.recordInsert(0, "Hello") == true);
    CHECK(ur.canUndo() == true);
    CHECK(ur.canRedo() == false);
}

// Expected: Undoing an insert removes the inserted text from the document.
void test_undo_insert_removes_text()
{
    UndoRedo ur;
    std::string doc = "Hello World";

    // Simulating that " World" was just inserted at index 5
    ur.recordInsert(5, " World");

    CHECK(ur.undo(doc) == true);
    CHECK(doc == "Hello");
    CHECK(ur.canUndo() == false);
    CHECK(ur.canRedo() == true);
}

// Expected: Redoing an undo of an insert puts the text back.
void test_redo_insert_restores_text()
{
    UndoRedo ur;
    std::string doc = "Hello World";
    ur.recordInsert(5, " World");
    ur.undo(doc);

    CHECK(ur.redo(doc) == true);
    CHECK(doc == "Hello World");
    CHECK(ur.canUndo() == true);
    CHECK(ur.canRedo() == false);
}

// Expected: Recording an erase makes canUndo true.
void test_record_erase_updates_state()
{
    UndoRedo ur;
    CHECK(ur.recordErase(0, "Test") == true);
    CHECK(ur.canUndo() == true);
}

// Expected: Undoing an erase puts the erased text back into the document.
// Expected: Undoing an erase puts the erased text back into the document.
void test_undo_erase_restores_text()
{
    UndoRedo ur;
    std::string doc = ""; // Document is empty because it was erased

    // Simulating that "Hello" was erased at index 0
    ur.recordErase(0, "Hello");

    CHECK(ur.undo(doc) == true);
    CHECK(doc == "Hello");
}

// Expected: Redoing an undo of an erase removes the text again.
// Expected: Redoing an undo of an erase removes the text again.
void test_redo_erase_removes_text()
{
    UndoRedo ur;
    std::string doc = "";
    ur.recordErase(0, "Hello");
    ur.undo(doc); // Doc is now "Hello"

    CHECK(ur.redo(doc) == true);
    CHECK(doc == "");
}

// Expected: Recording a replace makes canUndo true.
void test_record_replace_updates_state()
{
    UndoRedo ur;
    CHECK(ur.recordReplace(0, "old", "new") == true);
    CHECK(ur.canUndo() == true);
}

// Expected: Undoing a replace puts the old text back.
void test_undo_replace_restores_old_text()
{
    UndoRedo ur;
    std::string doc = "new text";

    // Simulating replacing "old" with "new" at index 0
    ur.recordReplace(0, "old", "new");

    CHECK(ur.undo(doc) == true);
    CHECK(doc == "old text");
}

// Expected: Redoing an undo of a replace applies the new text again.
void test_redo_replace_applies_new_text()
{
    UndoRedo ur;
    std::string doc = "new text";
    ur.recordReplace(0, "old", "new");
    ur.undo(doc); // Doc is now "old text"

    CHECK(ur.redo(doc) == true);
    CHECK(doc == "new text");
}

// Expected: Performing a new action after an undo clears the redo stack.
void test_new_action_clears_redo_stack()
{
    UndoRedo ur;
    std::string doc = "Hello";
    ur.recordInsert(5, " World");
    ur.undo(doc);

    CHECK(ur.canRedo() == true);

    // New action should wipe out the redo stack
    ur.recordInsert(0, "Start ");
    CHECK(ur.canRedo() == false);
}

// Expected: Calling undo or redo on an empty stack fails safely without crashing.
void test_boundary_empty_undo_redo()
{
    UndoRedo ur;
    std::string doc = "Unchanged";

    CHECK(ur.undo(doc) == false);
    CHECK(doc == "Unchanged"); // Document must not be modified

    CHECK(ur.redo(doc) == false);
    CHECK(doc == "Unchanged");
}

// Expected: Recording an operation with a negative position fails safely.
void test_boundary_invalid_position()
{
    UndoRedo ur;
    CHECK(ur.recordInsert(-1, "Invalid") == false);
    CHECK(ur.recordErase(-5, "Invalid") == false);
    CHECK(ur.recordReplace(-2, "a", "b") == false);
    CHECK(ur.canUndo() == false); // Nothing should be recorded
}

// Expected: clearHistory removes all undo and redo operations.
void test_clear_history()
{
    UndoRedo ur;
    std::string doc = "Text";
    ur.recordInsert(0, "Text");
    ur.undo(doc);

    CHECK(ur.canUndo() == false);
    CHECK(ur.canRedo() == true);

    ur.clearHistory();

    CHECK(ur.canUndo() == false);
    CHECK(ur.canRedo() == false);
}

int main()
{
    test_initial_state_empty();
    test_record_insert_updates_state();
    test_undo_insert_removes_text();
    test_redo_insert_restores_text();
    test_record_erase_updates_state();
    test_undo_erase_restores_text();
    test_redo_erase_removes_text();
    test_record_replace_updates_state();
    test_undo_replace_restores_old_text();
    test_redo_replace_applies_new_text();
    test_new_action_clears_redo_stack();
    test_boundary_empty_undo_redo();
    test_boundary_invalid_position();
    test_clear_history();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}