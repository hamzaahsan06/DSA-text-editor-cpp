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

// Reads an entire file into a string; returns "" if the file can't be opened.
// Opened in text mode so Windows normalizes "\r\n" back to "\n".
static std::string readRawFile(const std::string& path)
{
    std::ifstream in(path.c_str());    // text mode
    if (!in.is_open()) return "";
    std::string content;
    std::string line;
    bool first = true;
    while (std::getline(in, line))
    {
        if (!first) content += "\n";
        content += line;
        first = false;
    }
    return content;
}

// ---------- newDocument ----------

void test_new_document_clears()
{
    const std::string lines[] = { "a", "b" };
    Editor ed; makeEditorWith(lines, 2, ed);
    ed.setFilename("some.txt");
    ed.setModified(true);
    CHECK(newDocument(ed) == true);
    CHECK(ed.isEmpty() == true);
    CHECK(ed.getFilename() == "");
    CHECK(ed.isModified() == false);
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
    CHECK(ed.getLineCount() == 0);
    CHECK(ed.isModified() == false);

    std::remove(path.c_str());
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
    CHECK(readRawFile(path) == "alpha\nbeta\ngamma");

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
    CHECK(readRawFile(path) == "x\ny");

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

    test_open_empty_path();
    test_open_missing_file();
    test_open_reads_lines();
    test_open_empty_file();
    test_open_strips_cr();
    test_open_no_trailing_newline();

    test_save_without_filename();
    test_save_to_existing_filename();

    test_save_as_empty_path();
    test_save_as_writes_and_sets_filename();
    test_save_as_bad_path();

    test_save_then_open_round_trip();
    test_save_then_open_empty_middle_line();

    test_exit_unmodified();
    test_exit_modified();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}