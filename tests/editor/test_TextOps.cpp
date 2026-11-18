// g++ -std=c++17 -Wall -Wextra -Iinclude -Iinclude/editor tests/editor/test_TextOps.cpp src/editor/TextOps.cpp src/editor/UndoRedo.cpp src/editor/Bookmarks.cpp src/editor/Validation.cpp -o build/test_TextOps ; if ($?) { ./build/test_TextOps }

#include <iostream>
#include <string>
#include "editor/TextOps.h"
#include "editor/UndoRedo.h"
#include "editor/Bookmarks.h"
#include "editor/Editor.h"
#include "editor/Types.h"
#include "../ds/check.h"

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

// ---------- insertText ----------

void test_insert_text_middle()
{
    const std::string lines[] = { "abcde" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(insertText(ed, 0, 2, "XY") == true);
    CHECK(ed.getLine(0)->text == "abXYcde");
    CHECK(ed.cursorLine == 0);
    CHECK(ed.cursorCol == 4);
    CHECK(ed.isModified() == true);
}

void test_insert_text_at_col_zero()
{
    const std::string lines[] = { "abc" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(insertText(ed, 0, 0, "ZZ") == true);
    CHECK(ed.getLine(0)->text == "ZZabc");
    CHECK(ed.cursorCol == 2);
}

void test_insert_text_at_end()
{
    const std::string lines[] = { "abc" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(insertText(ed, 0, 3, "!") == true);
    CHECK(ed.getLine(0)->text == "abc!");
    CHECK(ed.cursorCol == 4);
}

void test_insert_text_empty_string_rejected()
{
    const std::string lines[] = { "abc" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(insertText(ed, 0, 0, "") == false);
    CHECK(ed.getLine(0)->text == "abc");
}

void test_insert_text_newline_rejected()
{
    const std::string lines[] = { "abc" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(insertText(ed, 0, 0, "a\nb") == false);
    CHECK(ed.getLine(0)->text == "abc");
}

void test_insert_text_bad_position()
{
    const std::string lines[] = { "abc" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(insertText(ed, 5, 0, "X") == false);
    CHECK(insertText(ed, 0, 99, "X") == false);
    CHECK(ed.getLine(0)->text == "abc");
}

void test_insert_text_empty_document_creates_line()
{
    Editor ed;
    ed.document.clear();
    CHECK(ed.getLineCount() == 0);
    CHECK(insertText(ed, 0, 0, "hi") == true);
    CHECK(ed.getLineCount() == 1);
    CHECK(ed.getLine(0)->text == "hi");
}

void test_insert_text_empty_document_rejects_other_positions()
{
    Editor ed;
    ed.document.clear();
    CHECK(insertText(ed, 1, 0, "x") == false);
    CHECK(insertText(ed, 0, 1, "x") == false);
    CHECK(ed.getLineCount() == 0);
}

void test_insert_text_second_line()
{
    const std::string lines[] = { "aaa", "bbb" };
    Editor ed; makeEditorWith(lines, 2, ed);
    CHECK(insertText(ed, 1, 1, "X") == true);
    CHECK(ed.getLine(0)->text == "aaa");
    CHECK(ed.getLine(1)->text == "bXbb");
    CHECK(ed.cursorLine == 1);
    CHECK(ed.cursorCol == 2);
}

void test_insert_text_shifts_formats()
{
    const std::string lines[] = { "abcdef" };
    Editor ed; makeEditorWith(lines, 1, ed);
    Line* l = ed.getLine(0);
    
    FormatSpan s; s.startCol = 2; s.endCol = 5; s.type = BOLD;
    l->formats.pushBack(s);

    CHECK(insertText(ed, 0, 1, "XX") == true);
    CHECK(l->text == "aXXbcdef");
    
    CHECK(l->formats.size() == 1);
    FormatSpan out;
    CHECK(l->formats.get(0, out));
    CHECK(out.startCol == 4);
    CHECK(out.endCol == 7);
}

// ---------- insertNewLine ----------

void test_insert_new_line_mid()
{
    const std::string lines[] = { "abcdef" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(insertNewLine(ed, 0, 3) == true);
    CHECK(ed.getLineCount() == 2);
    CHECK(ed.getLine(0)->text == "abc");
    CHECK(ed.getLine(1)->text == "def");
    CHECK(ed.cursorLine == 1);
    CHECK(ed.cursorCol == 0);
}

void test_insert_new_line_at_col_zero()
{
    const std::string lines[] = { "abc" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(insertNewLine(ed, 0, 0) == true);
    CHECK(ed.getLineCount() == 2);
    CHECK(ed.getLine(0)->text == "");
    CHECK(ed.getLine(1)->text == "abc");
}

void test_insert_new_line_at_end()
{
    const std::string lines[] = { "abc" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(insertNewLine(ed, 0, 3) == true);
    CHECK(ed.getLineCount() == 2);
    CHECK(ed.getLine(0)->text == "abc");
    CHECK(ed.getLine(1)->text == "");
}

void test_insert_new_line_bad_position()
{
    const std::string lines[] = { "abc" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(insertNewLine(ed, 5, 0) == false);
    CHECK(insertNewLine(ed, 0, 99) == false);
    CHECK(ed.getLineCount() == 1);
}

void test_insert_new_line_empty_document()
{
    Editor ed;
    ed.document.clear();
    CHECK(insertNewLine(ed, 0, 0) == true);
    CHECK(ed.getLineCount() == 2);
    CHECK(ed.getLine(0)->text == "");
    CHECK(ed.getLine(1)->text == "");
}

void test_insert_new_line_undo_removes_correct_line()
{
    const std::string lines[] = { "hello" };
    Editor ed; makeEditorWith(lines, 1, ed);
    
    CHECK(insertNewLine(ed, 0, 2) == true);
    CHECK(ed.getLineCount() == 2);
    CHECK(ed.getLine(0)->text == "he");
    CHECK(ed.getLine(1)->text == "llo");

    UndoRedo ur;
    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLineCount() == 1);
    CHECK(ed.getLine(0)->text == "hello");
}

// ---------- deleteText ----------

void test_delete_text_mid()
{
    const std::string lines[] = { "abcdef" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(deleteText(ed, 0, 1, 4) == true);
    CHECK(ed.getLine(0)->text == "aef");
    CHECK(ed.cursorLine == 0);
    CHECK(ed.cursorCol == 1);
    CHECK(ed.isModified() == true);
}

void test_delete_text_whole_line()
{
    const std::string lines[] = { "abc" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(deleteText(ed, 0, 0, 3) == true);
    CHECK(ed.getLine(0)->text == "");
    CHECK(ed.cursorCol == 0);
}

void test_delete_text_empty_range_rejected()
{
    const std::string lines[] = { "abc" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(deleteText(ed, 0, 1, 1) == false);
    CHECK(ed.getLine(0)->text == "abc");
}

void test_delete_text_bad_range()
{
    const std::string lines[] = { "abc" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(deleteText(ed, 5, 0, 1) == false);
    CHECK(deleteText(ed, 0, 1, 99) == false);
    CHECK(deleteText(ed, 0, 2, 1) == false);
    CHECK(ed.getLine(0)->text == "abc");
}

void test_delete_text_shifts_formats()
{
    const std::string lines[] = { "abcdef" };
    Editor ed; makeEditorWith(lines, 1, ed);
    Line* l = ed.getLine(0);
    
    FormatSpan s1; s1.startCol = 0; s1.endCol = 2; s1.type = BOLD;
    FormatSpan s2; s2.startCol = 2; s2.endCol = 4; s2.type = ITALIC;
    FormatSpan s3; s3.startCol = 4; s3.endCol = 6; s3.type = BOLD_ITALIC;
    
    l->formats.pushBack(s1);
    l->formats.pushBack(s2);
    l->formats.pushBack(s3);

    CHECK(deleteText(ed, 0, 2, 4) == true);
    CHECK(l->text == "abef");
    
    CHECK(l->formats.size() == 2);
    
    FormatSpan out1, out2;
    CHECK(l->formats.get(0, out1));
    CHECK(l->formats.get(1, out2));
    
    CHECK(out1.startCol == 0 && out1.endCol == 2);
    CHECK(out2.startCol == 2 && out2.endCol == 4);
}

// ---------- deleteChar ----------

void test_delete_char_valid()
{
    const std::string lines[] = { "abc" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(deleteChar(ed, 0, 1) == true);
    CHECK(ed.getLine(0)->text == "ac");
}

void test_delete_char_bad_col()
{
    const std::string lines[] = { "abc" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(deleteChar(ed, 0, -1) == false);
    CHECK(deleteChar(ed, 0, 3) == false);
    CHECK(ed.getLine(0)->text == "abc");
}

void test_delete_char_bad_line()
{
    const std::string lines[] = { "abc" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(deleteChar(ed, 99, 0) == false);
}

// ---------- deleteWord ----------

void test_delete_word_middle()
{
    const std::string lines[] = { "hello world foo" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(deleteWord(ed, 0, 8) == true);
    CHECK(ed.getLine(0)->text == "hello  foo");
}

void test_delete_word_first()
{
    const std::string lines[] = { "one two three" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(deleteWord(ed, 0, 1) == true);
    CHECK(ed.getLine(0)->text == " two three");
}

void test_delete_word_last()
{
    const std::string lines[] = { "one two three" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(deleteWord(ed, 0, 12) == true);
    CHECK(ed.getLine(0)->text == "one two ");
}

void test_delete_word_on_whitespace_rejected()
{
    const std::string lines[] = { "one two" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(deleteWord(ed, 0, 3) == false);
    CHECK(ed.getLine(0)->text == "one two");
}

void test_delete_word_bad_col()
{
    const std::string lines[] = { "one" };
    Editor ed; makeEditorWith(lines, 1, ed);
    CHECK(deleteWord(ed, 0, -1) == false);
    CHECK(deleteWord(ed, 0, 99) == false);
}

// ---------- deleteLines ----------

void test_delete_lines_one_middle()
{
    const std::string lines[] = { "a", "b", "c" };
    Editor ed; makeEditorWith(lines, 3, ed);
    CHECK(deleteLines(ed, 1, 1) == true);
    CHECK(ed.getLineCount() == 2);
    CHECK(ed.getLine(0)->text == "a");
    CHECK(ed.getLine(1)->text == "c");
}

void test_delete_lines_range()
{
    const std::string lines[] = { "a", "b", "c", "d", "e" };
    Editor ed; makeEditorWith(lines, 5, ed);
    CHECK(deleteLines(ed, 1, 3) == true);
    CHECK(ed.getLineCount() == 2);
    CHECK(ed.getLine(0)->text == "a");
    CHECK(ed.getLine(1)->text == "e");
}

void test_delete_lines_whole_document()
{
    const std::string lines[] = { "a", "b", "c" };
    Editor ed; makeEditorWith(lines, 3, ed);
    CHECK(deleteLines(ed, 0, 2) == true);
    CHECK(ed.getLineCount() == 0);
    CHECK(ed.cursorLine == 0);
    CHECK(ed.cursorCol == 0);
}

void test_delete_lines_bad_range()
{
    const std::string lines[] = { "a", "b" };
    Editor ed; makeEditorWith(lines, 2, ed);
    CHECK(deleteLines(ed, 5, 6) == false);
    CHECK(deleteLines(ed, 1, 0) == false);
    CHECK(ed.getLineCount() == 2);
}

void test_delete_lines_empty_document()
{
    Editor ed;
    ed.document.clear();
    CHECK(deleteLines(ed, 0, 0) == false);
}

void test_delete_lines_records_undo()
{
    const std::string lines[] = { "one", "two", "three" };
    Editor ed; makeEditorWith(lines, 3, ed);
    deleteLines(ed, 1, 2);
    CHECK(ed.undoStack.size() == 1);
    Command c;
    CHECK(ed.undoStack.peek(c) == true);
    CHECK(c.type == DELETE_LINE);
    CHECK(c.line == 1);
    CHECK(c.oldText == "two\nthree");
}

void test_delete_lines_undo_restores()
{
    const std::string lines[] = { "one", "two", "three", "four" };
    Editor ed; makeEditorWith(lines, 4, ed);
    
    CHECK(deleteLines(ed, 1, 2) == true);
    CHECK(ed.getLineCount() == 2);
    CHECK(ed.getLine(0)->text == "one");
    CHECK(ed.getLine(1)->text == "four");

    UndoRedo ur;
    CHECK(ur.undo(ed) == true);
    CHECK(ed.getLineCount() == 4);
    CHECK(ed.getLine(0)->text == "one");
    CHECK(ed.getLine(1)->text == "two");
    CHECK(ed.getLine(2)->text == "three");
    CHECK(ed.getLine(3)->text == "four");
}

// ---------- interactions with UndoRedo / Bookmarks ----------

void test_insert_text_records_undo()
{
    const std::string lines[] = { "abc" };
    Editor ed; makeEditorWith(lines, 1, ed);
    insertText(ed, 0, 1, "XY");
    CHECK(ed.undoStack.size() == 1);
    Command c;
    CHECK(ed.undoStack.peek(c) == true);
    CHECK(c.type == INSERT_TEXT);
    CHECK(c.line == 0);
    CHECK(c.col == 1);
    CHECK(c.newText == "XY");
}

void test_delete_text_records_undo()
{
    const std::string lines[] = { "abcdef" };
    Editor ed; makeEditorWith(lines, 1, ed);
    deleteText(ed, 0, 2, 5);
    CHECK(ed.undoStack.size() == 1);
    Command c;
    CHECK(ed.undoStack.peek(c) == true);
    CHECK(c.type == DELETE_TEXT);
    CHECK(c.line == 0);
    CHECK(c.col == 2);
    CHECK(c.oldText == "cde");
}

void test_insert_new_line_adjusts_bookmarks()
{
    const std::string lines[] = { "abcdef" };
    Editor ed; makeEditorWith(lines, 1, ed);

    Bookmark b0;
    b0.name = "first";
    b0.line = 0; b0.col = 0;
    ed.bookmarks.pushBack(b0);

    CHECK(insertNewLine(ed, 0, 3) == true);

    int idx = findBookmark(ed, "first");
    CHECK(idx == 0);
    Bookmark b;
    CHECK(ed.bookmarks.get(idx, b) == true);
    CHECK(b.line == 0);
}

void test_delete_lines_adjusts_bookmarks()
{
    const std::string lines[] = { "a", "b", "c" };
    Editor ed; makeEditorWith(lines, 3, ed);

    Bookmark onB;
    onB.name = "onB";
    onB.line = 1; onB.col = 0;
    ed.bookmarks.pushBack(onB);

    Bookmark onC;
    onC.name = "onC";
    onC.line = 2; onC.col = 0;
    ed.bookmarks.pushBack(onC);

    CHECK(deleteLines(ed, 1, 1) == true);
    CHECK(findBookmark(ed, "onB") == -1);
    int idxC = findBookmark(ed, "onC");
    CHECK(idxC >= 0);
    Bookmark c;
    CHECK(ed.bookmarks.get(idxC, c) == true);
    CHECK(c.line == 1);
}

int main()
{
    test_insert_text_middle();
    test_insert_text_at_col_zero();
    test_insert_text_at_end();
    test_insert_text_empty_string_rejected();
    test_insert_text_newline_rejected();
    test_insert_text_bad_position();
    test_insert_text_empty_document_creates_line();
    test_insert_text_empty_document_rejects_other_positions();
    test_insert_text_second_line();
    test_insert_text_shifts_formats();

    test_insert_new_line_mid();
    test_insert_new_line_at_col_zero();
    test_insert_new_line_at_end();
    test_insert_new_line_bad_position();
    test_insert_new_line_empty_document();
    test_insert_new_line_undo_removes_correct_line();

    test_delete_text_mid();
    test_delete_text_whole_line();
    test_delete_text_empty_range_rejected();
    test_delete_text_bad_range();
    test_delete_text_shifts_formats();

    test_delete_char_valid();
    test_delete_char_bad_col();
    test_delete_char_bad_line();

    test_delete_word_middle();
    test_delete_word_first();
    test_delete_word_last();
    test_delete_word_on_whitespace_rejected();
    test_delete_word_bad_col();

    test_delete_lines_one_middle();
    test_delete_lines_range();
    test_delete_lines_whole_document();
    test_delete_lines_bad_range();
    test_delete_lines_empty_document();
    test_delete_lines_records_undo();
    test_delete_lines_undo_restores();

    test_insert_text_records_undo();
    test_delete_text_records_undo();
    test_insert_new_line_adjusts_bookmarks();
    test_delete_lines_adjusts_bookmarks();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}