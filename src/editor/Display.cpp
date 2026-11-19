#include "editor/Display.h"

typedef DoublyLinkedList<Line>::Node Node;

// True for space, tab, newline, carriage return, vertical tab and form feed.
static bool isWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

// Number of digits in n (n >= 0).
static int digitCount(int n)
{
    int d = 1;
    while (n >= 10)
    {
        n /= 10;
        ++d;
    }
    return d;
}

// Print "  7 | " with the number right-aligned in `width` columns.
static void printLabel(std::ostream &out, int number, int width)
{
    for (int i = digitCount(number); i < width; ++i)
        out << ' ';
    out << number << " | ";
}

// Node at index, walking from the nearer end. Returns nullptr if out of range.
// Used instead of DoublyLinkedList::get() so the Line (and its formats) is not copied.
static Node *nodeAtIndex(const DoublyLinkedList<Line> &doc, int index)
{
    int n = doc.size();
    if (index < 0 || index >= n)
        return nullptr;

    Node *cur;
    if (index < n / 2)
    {
        cur = doc.getHead();
        for (int i = 0; i < index; ++i)
            cur = cur->next;
    }
    else
    {
        cur = doc.getTail();
        for (int i = n - 1; i > index; --i)
            cur = cur->prev;
    }
    return cur;
}

// Compute the 0-based start and end line indices for a window of size maxLines.
static void computeWindowBounds(int totalLines, int cursorLine, int maxLines, int &startIdx, int &endIdx)
{
    int clampedCursor = cursorLine;
    if (clampedCursor < 0)
        clampedCursor = 0;
    if (clampedCursor >= totalLines)
        clampedCursor = totalLines - 1;

    int halfBefore = (maxLines - 1) / 2;
    startIdx = clampedCursor - halfBefore;
    if (startIdx < 0)
        startIdx = 0;

    endIdx = startIdx + maxLines - 1;
    if (endIdx >= totalLines)
    {
        endIdx = totalLines - 1;
        startIdx = endIdx - maxLines + 1;
        if (startIdx < 0)
            startIdx = 0;
    }
}

DocStats getStats(const Editor &ed)
{
    DocStats s;
    s.lines = ed.document.size();
    s.words = 0;
    s.chars = 0;
    s.charsNoSpaces = 0;

    for (Node *cur = ed.document.getHead(); cur != nullptr; cur = cur->next)
    {
        const std::string &text = cur->data.text;
        s.chars += static_cast<int>(text.size());

        bool inWord = false; // words never span two lines
        for (std::size_t i = 0; i < text.size(); ++i)
        {
            if (isWhitespace(text[i]))
            {
                inWord = false;
            }
            else
            {
                ++s.charsNoSpaces;
                if (!inWord)
                {
                    ++s.words;
                    inWord = true;
                }
            }
        }
    }
    return s;
}

void displayDocument(const Editor &ed, std::ostream &out)
{
    if (ed.document.isEmpty())
    {
        out << "(document is empty)\n";
        return;
    }

    int width = digitCount(ed.document.size());
    int number = 1;
    // Follow node pointers: O(n). Calling get(i) in a loop would be O(n^2).
    for (Node *cur = ed.document.getHead(); cur != nullptr; cur = cur->next, ++number)
    {
        printLabel(out, number, width);
        out << cur->data.text << '\n';
    }
}

bool displayLine(const Editor &ed, int index, std::ostream &out)
{
    if (ed.document.isEmpty())
    {
        out << "Error: the document is empty.\n";
        return false;
    }

    Node *node = nodeAtIndex(ed.document, index);
    if (node == nullptr)
    {
        out << "Error: line " << (index + 1) << " does not exist (document has "
            << ed.document.size() << " lines).\n";
        return false;
    }

    printLabel(out, index + 1, digitCount(ed.document.size()));
    out << node->data.text << '\n';
    return true;
}

void displayStats(const Editor &ed, std::ostream &out)
{
    DocStats s = getStats(ed);
    std::string filename = ed.getFilename();

    out << "--- Document info ---\n";
    out << "File       : " << (filename.empty() ? "(unsaved)" : filename) << '\n';
    out << "Lines      : " << s.lines << '\n';
    out << "Words      : " << s.words << '\n';
    out << "Characters : " << s.chars << " (" << s.charsNoSpaces << " without spaces)\n";
    out << "Modified   : " << (ed.isModified() ? "yes" : "no") << '\n';
}

void displayStatusBar(const Editor &ed, std::ostream &out)
{
    DocStats s = getStats(ed);
    std::string name = ed.getFilename();
    if (name.empty())
        name = "[untitled]";

    out << name;
    if (ed.isModified())
        out << " [modified]";
    out << " | ";

    if (s.lines == 0)
    {
        out << "Ln -, Col -";
    }
    else
    {
        out << "Ln " << (ed.cursorLine + 1) << "/" << s.lines
            << ", Col " << (ed.cursorCol + 1);
    }

    out << " | " << s.words << " words"
        << " | " << s.chars << " chars"
        << " | undo " << ed.undoStack.size()
        << ", redo " << ed.redoStack.size() << '\n';
}

bool displayRange(const Editor &ed, int startLine, int endLine, bool markCursor, std::ostream &out)
{
    int total = ed.document.size();
    if (total == 0)
    {
        out << "Error: the document is empty.\n";
        return false;
    }
    if (startLine < 0 || endLine < startLine || endLine >= total)
    {
        out << "Error: invalid line range " << (startLine + 1) << ".." << (endLine + 1)
            << " (document has " << total << " lines).\n";
        return false;
    }

    int width = digitCount(total);
    int idx = startLine;
    for (Node *cur = nodeAtIndex(ed.document, startLine); cur != nullptr && idx <= endLine; cur = cur->next, ++idx)
    {
        if (markCursor)
        {
            out << (idx == ed.cursorLine ? '>' : ' ');
        }
        printLabel(out, idx + 1, width);
        out << cur->data.text << '\n';
    }
    return true;
}

void displayWindow(const Editor &ed, int maxLines, std::ostream &out)
{
    int total = ed.document.size();
    if (total == 0)
    {
        out << "(document is empty)\n";
        return;
    }
    if (maxLines <= 0)
    {
        out << "... " << total << " more line(s) below\n";
        return;
    }
    if (total <= maxLines)
    {
        displayRange(ed, 0, total - 1, true, out);
        return;
    }

    int startIdx = 0;
    int endIdx = 0;
    computeWindowBounds(total, ed.cursorLine, maxLines, startIdx, endIdx);

    if (startIdx > 0)
    {
        out << "... " << startIdx << " more line(s) above\n";
    }
    displayRange(ed, startIdx, endIdx, true, out);
    if (endIdx < total - 1)
    {
        out << "... " << (total - 1 - endIdx) << " more line(s) below\n";
    }
}