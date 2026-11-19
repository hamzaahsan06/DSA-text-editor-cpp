// g++ -std=c++17 -Wall -Wextra -Iinclude tests/editor/test_Display.cpp src/editor/Display.cpp -o test_Display

#include <iostream>
#include <sstream>
#include <string>
#include "editor/Display.h"

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

// Append one line of text to the editor's document.
static void addLine(Editor& ed, const std::string& text) {
    Line l;
    l.text = text;
    ed.document.insertAtTail(l);
}

// Make a document with exactly these lines: "hello world", "", "  a  b   c ".
static void fillSample(Editor& ed) {
    ed.document.clear();
    addLine(ed, "hello world");
    addLine(ed, "");
    addLine(ed, "  a  b   c ");
}

// Expected: a new Editor starts with one empty line, so it shows as line 1.
void test_fresh_editor() {
    Editor ed;
    std::ostringstream out;
    displayDocument(ed, out);
    CHECK(out.str() == "1 | \n");

    DocStats s = getStats(ed);
    CHECK(s.lines == 1 && s.words == 0 && s.chars == 0 && s.charsNoSpaces == 0);
}

// Expected: a document with no lines prints the empty message and all-zero stats.
void test_no_lines() {
    Editor ed;
    ed.document.clear();

    std::ostringstream out;
    displayDocument(ed, out);
    CHECK(out.str() == "(document is empty)\n");

    DocStats s = getStats(ed);
    CHECK(s.lines == 0 && s.words == 0 && s.chars == 0 && s.charsNoSpaces == 0);
}

// Expected: counts ignore blank lines and repeated spaces when counting words.
void test_stats_counts() {
    Editor ed;
    fillSample(ed);
    DocStats s = getStats(ed);
    CHECK(s.lines == 3);
    CHECK(s.words == 5);
    CHECK(s.chars == 22);
    CHECK(s.charsNoSpaces == 13);
}

// Expected: every line is printed with a 1-based number.
void test_display_document() {
    Editor ed;
    fillSample(ed);
    std::ostringstream out;
    displayDocument(ed, out);
    CHECK(out.str() == "1 | hello world\n2 | \n3 |   a  b   c \n");
}

// Expected: line numbers are right-aligned once there are 10 or more lines.
void test_number_alignment() {
    Editor ed;
    ed.document.clear();
    for (int i = 0; i < 10; ++i) addLine(ed, "x");
    std::ostringstream out;
    displayDocument(ed, out);
    CHECK(out.str().substr(0, 9) == " 1 | x\n 2");
}

// Expected: displayLine takes a 0-based index and shows a 1-based label.
void test_display_line_valid() {
    Editor ed;
    fillSample(ed);
    std::ostringstream out;
    CHECK(displayLine(ed, 0, out));
    CHECK(out.str() == "1 | hello world\n");

    std::ostringstream last;
    CHECK(displayLine(ed, 2, last));          // exercises the walk from the tail
    CHECK(last.str() == "3 |   a  b   c \n");
}

// Expected: bad indexes fail with a message and print no line.
void test_display_line_invalid() {
    Editor ed;
    fillSample(ed);

    std::ostringstream past;
    CHECK(!displayLine(ed, 3, past));
    CHECK(past.str().find("does not exist") != std::string::npos);

    std::ostringstream neg;
    CHECK(!displayLine(ed, -1, neg));

    Editor none;
    none.document.clear();
    std::ostringstream empty;
    CHECK(!displayLine(none, 0, empty));
    CHECK(empty.str().find("empty") != std::string::npos);
}

// Expected: stats output shows filename (or "(unsaved)"), counts and the modified flag.
void test_display_stats() {
    Editor ed;
    fillSample(ed);
    ed.setModified(true);

    std::ostringstream a;
    displayStats(ed, a);
    std::string t = a.str();
    CHECK(t.find("(unsaved)") != std::string::npos);
    CHECK(t.find("Lines      : 3") != std::string::npos);
    CHECK(t.find("Words      : 5") != std::string::npos);
    CHECK(t.find("Characters : 22 (13 without spaces)") != std::string::npos);
    CHECK(t.find("Modified   : yes") != std::string::npos);

    ed.setFilename("notes.txt");
    ed.setModified(false);
    std::ostringstream b;
    displayStats(ed, b);
    CHECK(b.str().find("File       : notes.txt") != std::string::npos);
    CHECK(b.str().find("Modified   : no") != std::string::npos);
}

int main() {
    test_fresh_editor();
    test_no_lines();
    test_stats_counts();
    test_display_document();
    test_number_alignment();
    test_display_line_valid();
    test_display_line_invalid();
    test_display_stats();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}