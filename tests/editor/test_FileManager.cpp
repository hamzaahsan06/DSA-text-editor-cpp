// g++ -std=c++17 -Wall -Wextra -Iinclude -Iinclude/editor tests/editor/test_FileManager.cpp src/editor/FileManager.cpp -o build/test_FileManager ; if ($?) { ./build/test_FileManager }

#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include "editor/FileManager.h"
#include "editor/Editor.h"
#include "editor/Types.h"
#include "../ds/check.h"

// Fills `ed` with the given lines. Editor is non-copyable, so fill in place.
static void makeEditorWith(const std::string* lines, int n, Editor& ed)
{
    ed.reset();
    if (n <= 0) return;
    Line* first = ed.getLine(0);
    if (first != nullptr) first->text = lines[0];
    for (int i = 1; i < n; ++i)
    {
        Line l;
        l.text = lines[i];
        ed.document.insertAtTail(l);
    }
}

// Writes a raw file with the given content (text mode, so on Windows
// '\n' becomes '\r\n' on disk — same as FileManager::writeToFile).
static void writeRawFile(const std::string& path, const std::string& content)
{
    std::ofstream out(path.c_str());   // text mode
    out << content;
    out.close();
}

// Reads an entire file into a string, exactly as stored (including a final
// '\n'); returns "" if the file can't be opened.
// Opened in text mode so Windows normalizes "\r\n" back to "\n".
static std::string readRawFile(const std::string& path)
{
    std::ifstream in(path.c_str());    // text mode
    if (!in.is_open()) return "";
    std::string content;
    char c;
    while (in.get(c))
        content += c;
    return content;
}

// ---------- newDocument ----------

// Puts `ed` into a "used" state: three lines, a filename, unsaved changes,
// cursor away from the origin, one undo command, one redo command, one bookmark.
static void makeDirty(Editor& ed)
{
    const std::string lines[] = { "a", "b", "c" };
    makeEditorWith(lines, 3, ed);
    ed.setFilename("old.txt");
    ed.setModified(true);
    ed.cursorLine = 2;
    ed.cursorCol = 1;

    Command cmd;
    cmd.type = INSERT_TEXT;
    ed.undoStack.push(cmd);
    ed.redoStack.push(cmd);

    Bookmark mark;
    mark.name = "m";
    mark.line = 1;
    ed.bookmarks.pushBack(mark);
}

// True if `ed` is in exactly the state Editor::reset() produces
// (one empty line, empty stacks, no bookmarks, cursor 0/0, untitled, unmodified).
static bool isResetState(Editor& ed)
{
    return ed.getLineCount() == 1
        && ed.getLine(0) != nullptr
        && ed.getLine(0)->text == ""
        && ed.getLine(0)->formats.size() == 0
        && ed.undoStack.isEmpty()
        && ed.redoStack.isEmpty()
        && ed.bookmarks.size() == 0
        && ed.cursorLine == 0
        && ed.cursorCol == 0
        && ed.getFilename() == ""
        && ed.isModified() == false;
}

void test_new_document_clears()
{
    const std::string lines[] = { "a", "b" };
    Editor ed; makeEditorWith(lines, 2, ed);
    ed.setFilename("some.txt");
    ed.setModified(true);
    CHECK(newDocument(ed) == true);
    CHECK(ed.getLineCount() == 1);          // one empty line, not zero lines
    CHECK(ed.getLine(0)->text == "");
    CHECK(ed.getFilename() == "");
    CHECK(ed.isModified() == false);
}

// newDocument must leave the editor exactly as Editor::reset() does.
void test_new_document_equals_reset_state()
{
    Editor ed; makeDirty(ed);
    CHECK(isResetState(ed) == false);       // sanity: it really is dirty
    CHECK(newDocument(ed) == true);
    CHECK(isResetState(ed) == true);

    Editor fresh;                           // a brand-new editor is the reference
    CHECK(isResetState(fresh) == true);
}

// ---------- openFile ----------

void test_open_empty_path()
{
    const std::string lines[] = { "keep" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(openFile(ed, "") == false);
    CHECK(ed.getLineCount() == 1);
    CHECK(ed.getLine(0)->text == "keep");
}

void test_open_missing_file()
{
    const std::string lines[] = { "keep" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(openFile(ed, "definitely_missing_file_xyz.txt") == false);
    CHECK(ed.getLineCount() == 1);
    CHECK(ed.getLine(0)->text == "keep");
}

void test_open_reads_lines()
{
    const std::string path = "test_open_reads_lines_tmp.txt";
    writeRawFile(path, "hello\nworld\nthird");

    Editor ed;
    CHECK(openFile(ed, path) == true);
    CHECK(ed.getLineCount() == 3);
    CHECK(ed.getLine(0)->text == "hello");
    CHECK(ed.getLine(1)->text == "world");
    CHECK(ed.getLine(2)->text == "third");
    CHECK(ed.getFilename() == path);
    CHECK(ed.isModified() == false);

    std::remove(path.c_str());
}

void test_open_empty_file()
{
    const std::string path = "test_open_empty_file_tmp.txt";
    writeRawFile(path, "");

    Editor ed;
    CHECK(openFile(ed, path) == true);
    CHECK(ed.getLineCount() == 1);          // an empty file gives one empty line
    CHECK(ed.getLine(0)->text == "");
    CHECK(ed.getFilename() == path);
    CHECK(ed.isModified() == false);

    std::remove(path.c_str());
}

// Opening an empty file over a used editor must look like a new document,
// except that it keeps the file name.
void test_open_empty_file_matches_new_document()
{
    const std::string path = "test_open_empty_vs_new_tmp.txt";
    writeRawFile(path, "");

    Editor ed; makeDirty(ed);
    CHECK(openFile(ed, path) == true);
    CHECK(ed.getFilename() == path);
    ed.setFilename("");                     // ignore the name; compare the rest
    CHECK(isResetState(ed) == true);

    std::remove(path.c_str());
}

// Undo and redo history belong to the old file, so opening must clear both.
void test_open_clears_undo_history()
{
    const std::string path = "test_open_clears_history_tmp.txt";
    writeRawFile(path, "new\ncontent");

    Editor ed; makeDirty(ed);
    CHECK(ed.undoStack.isEmpty() == false);
    CHECK(ed.redoStack.isEmpty() == false);
    CHECK(openFile(ed, path) == true);
    CHECK(ed.undoStack.isEmpty() == true);
    CHECK(ed.redoStack.isEmpty() == true);

    std::remove(path.c_str());
}

// Bookmarks point into the old document, so opening must clear them.
void test_open_clears_bookmarks()
{
    const std::string path = "test_open_clears_bookmarks_tmp.txt";
    writeRawFile(path, "new\ncontent");

    Editor ed; makeDirty(ed);
    CHECK(ed.bookmarks.size() == 1);
    CHECK(openFile(ed, path) == true);
    CHECK(ed.bookmarks.size() == 0);

    std::remove(path.c_str());
}

// The cursor must go back to the start of the newly opened document.
void test_open_resets_cursor_and_state()
{
    const std::string path = "test_open_resets_cursor_tmp.txt";
    writeRawFile(path, "one\ntwo");

    Editor ed; makeDirty(ed);
    CHECK(openFile(ed, path) == true);
    CHECK(ed.cursorLine == 0);
    CHECK(ed.cursorCol == 0);
    CHECK(ed.getFilename() == path);
    CHECK(ed.isModified() == false);
    CHECK(ed.getLineCount() == 2);
    CHECK(ed.getLine(0)->text == "one");
    CHECK(ed.getLine(1)->text == "two");

    std::remove(path.c_str());
}

// If opening fails, NOTHING about the current editor may change:
// document, history, bookmarks, cursor, filename and modified flag.
void test_failed_open_keeps_everything()
{
    Editor ed; makeDirty(ed);

    CHECK(openFile(ed, "definitely_missing_file_xyz.txt") == false);
    CHECK(openFile(ed, "") == false);

    CHECK(ed.getLineCount() == 3);
    CHECK(ed.getLine(0)->text == "a");
    CHECK(ed.getLine(1)->text == "b");
    CHECK(ed.getLine(2)->text == "c");
    CHECK(ed.undoStack.size() == 1);
    CHECK(ed.redoStack.size() == 1);
    CHECK(ed.bookmarks.size() == 1);
    CHECK(ed.cursorLine == 2);
    CHECK(ed.cursorCol == 1);
    CHECK(ed.getFilename() == "old.txt");
    CHECK(ed.isModified() == true);
}

void test_open_strips_cr()
{
    const std::string path = "test_open_strips_cr_tmp.txt";
    writeRawFile(path, "alpha\r\nbeta\r\n");

    Editor ed;
    CHECK(openFile(ed, path) == true);
    CHECK(ed.getLineCount() == 2);
    CHECK(ed.getLine(0)->text == "alpha");
    CHECK(ed.getLine(1)->text == "beta");

    std::remove(path.c_str());
}

void test_open_no_trailing_newline()
{
    const std::string path = "test_open_no_trailing_tmp.txt";
    writeRawFile(path, "one\ntwo");

    Editor ed;
    CHECK(openFile(ed, path) == true);
    CHECK(ed.getLineCount() == 2);
    CHECK(ed.getLine(0)->text == "one");
    CHECK(ed.getLine(1)->text == "two");

    std::remove(path.c_str());
}

// ---------- saveFile ----------

void test_save_without_filename()
{
    const std::string lines[] = { "a" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(ed.getFilename() == "");
    CHECK(saveFile(ed) == false);
}

void test_save_to_existing_filename()
{
    const std::string path = "test_save_to_existing_tmp.txt";
    const std::string lines[] = { "alpha", "beta", "gamma" };
    Editor ed; makeEditorWith(lines, 3, ed);
    ed.setFilename(path);
    ed.setModified(true);

    CHECK(saveFile(ed) == true);
    CHECK(ed.isModified() == false);
    CHECK(readRawFile(path) == "alpha\nbeta\ngamma\n");   // every line ends with '\n'

    std::remove(path.c_str());
}

// ---------- saveFileAs ----------

void test_save_as_empty_path()
{
    const std::string lines[] = { "a" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(saveFileAs(ed, "") == false);
}

void test_save_as_writes_and_sets_filename()
{
    const std::string path = "test_save_as_tmp.txt";
    const std::string lines[] = { "x", "y" };
    Editor ed; makeEditorWith(lines, 2, ed);
    ed.setModified(true);

    CHECK(saveFileAs(ed, path) == true);
    CHECK(ed.getFilename() == path);
    CHECK(ed.isModified() == false);
    CHECK(readRawFile(path) == "x\ny\n");

    std::remove(path.c_str());
}

void test_save_as_bad_path()
{
    const std::string lines[] = { "keep" };
    Editor ed; makeEditorWith(lines, 1, ed);
    ed.setFilename("original.txt");
    ed.setModified(true);
    CHECK(saveFileAs(ed, "/nonexistent_dir_xyz/out.txt") == false);
    CHECK(ed.getFilename() == "original.txt");
    CHECK(ed.isModified() == true);
}

// ---------- round-trip ----------

void test_save_then_open_round_trip()
{
    const std::string path = "test_round_trip_tmp.txt";
    const std::string lines[] = { "one", "two", "three" };
    Editor ed; makeEditorWith(lines, 3, ed);
    CHECK(saveFileAs(ed, path) == true);

    Editor ed2;
    CHECK(openFile(ed2, path) == true);
    CHECK(ed2.getLineCount() == 3);
    CHECK(ed2.getLine(0)->text == "one");
    CHECK(ed2.getLine(1)->text == "two");
    CHECK(ed2.getLine(2)->text == "three");

    std::remove(path.c_str());
}

// Empty lines at the end (and in the middle) must survive save + reopen:
// the line count after reopening must equal the line count that was saved.
void test_save_then_open_keeps_trailing_empty_lines()
{
    const std::string path = "test_trailing_empty_tmp.txt";
    const std::string lines[] = { "a", "", "b", "", "" };
    Editor ed; makeEditorWith(lines, 5, ed);
    CHECK(saveFileAs(ed, path) == true);
    CHECK(readRawFile(path) == "a\n\nb\n\n\n");

    Editor ed2;
    CHECK(openFile(ed2, path) == true);
    CHECK(ed2.getLineCount() == 5);
    CHECK(ed2.getLine(0)->text == "a");
    CHECK(ed2.getLine(1)->text == "");
    CHECK(ed2.getLine(2)->text == "b");
    CHECK(ed2.getLine(3)->text == "");
    CHECK(ed2.getLine(4)->text == "");

    // Saving again must not change the file (no extra or missing lines each cycle).
    CHECK(saveFileAs(ed2, path) == true);
    CHECK(readRawFile(path) == "a\n\nb\n\n\n");

    std::remove(path.c_str());
}

// A file that does not end with a newline still opens fully, and is saved
// back with the final newline.
void test_open_file_without_final_newline_then_save()
{
    const std::string path = "test_no_final_newline_tmp.txt";
    writeRawFile(path, "one\ntwo");

    Editor ed;
    CHECK(openFile(ed, path) == true);
    CHECK(ed.getLineCount() == 2);
    CHECK(saveFile(ed) == true);
    CHECK(readRawFile(path) == "one\ntwo\n");

    std::remove(path.c_str());
}

// A new document (one empty line) saves as one newline and opens back
// as the same single empty line.
void test_save_then_open_new_document()
{
    const std::string path = "test_new_doc_round_trip_tmp.txt";
    Editor ed;
    CHECK(newDocument(ed) == true);
    CHECK(saveFileAs(ed, path) == true);
    CHECK(readRawFile(path) == "\n");

    Editor ed2; makeDirty(ed2);
    CHECK(openFile(ed2, path) == true);
    CHECK(ed2.getLineCount() == 1);
    CHECK(ed2.getLine(0)->text == "");

    std::remove(path.c_str());
}

// saveAs may change the filename and the modified flag only when it succeeds.
void test_save_as_changes_state_only_on_success()
{
    const std::string path = "test_save_as_state_tmp.txt";
    Editor ed; makeDirty(ed);               // filename "old.txt", modified

    CHECK(saveFileAs(ed, "/nonexistent_dir_xyz/out.txt") == false);
    CHECK(ed.getFilename() == "old.txt");
    CHECK(ed.isModified() == true);

    CHECK(saveFileAs(ed, path) == true);
    CHECK(ed.getFilename() == path);
    CHECK(ed.isModified() == false);

    std::remove(path.c_str());
}

void test_save_then_open_empty_middle_line()
{
    const std::string path = "test_empty_middle_tmp.txt";
    const std::string lines[] = { "top", "", "bottom" };
    Editor ed; makeEditorWith(lines, 3, ed);
    CHECK(saveFileAs(ed, path) == true);

    Editor ed2;
    CHECK(openFile(ed2, path) == true);
    CHECK(ed2.getLineCount() == 3);
    CHECK(ed2.getLine(0)->text == "top");
    CHECK(ed2.getLine(1)->text == "");
    CHECK(ed2.getLine(2)->text == "bottom");

    std::remove(path.c_str());
}

// ---------- exitEditor ----------

void test_exit_unmodified()
{
    Editor ed;
    ed.setModified(false);
    CHECK(exitEditor(ed) == true);
}

void test_exit_modified()
{
    Editor ed;
    ed.setModified(true);
    CHECK(exitEditor(ed) == false);
}

int main()
{
    test_new_document_clears();
    test_new_document_equals_reset_state();

    test_open_empty_path();
    test_open_missing_file();
    test_open_reads_lines();
    test_open_empty_file();
    test_open_empty_file_matches_new_document();
    test_open_strips_cr();
    test_open_no_trailing_newline();
    test_open_clears_undo_history();
    test_open_clears_bookmarks();
    test_open_resets_cursor_and_state();
    test_failed_open_keeps_everything();

    test_save_without_filename();
    test_save_to_existing_filename();

    test_save_as_empty_path();
    test_save_as_writes_and_sets_filename();
    test_save_as_bad_path();

    test_save_then_open_round_trip();
    test_save_then_open_empty_middle_line();
    test_save_then_open_new_document();
    test_save_then_open_keeps_trailing_empty_lines();
    test_open_file_without_final_newline_then_save();
    test_save_as_changes_state_only_on_success();

    test_exit_unmodified();
    test_exit_modified();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}