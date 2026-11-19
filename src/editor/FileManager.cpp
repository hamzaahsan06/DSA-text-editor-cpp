#include "editor/FileManager.h"

#include <fstream>
#include <iostream>

// Writes the document as plain text, keeping empty and trailing lines intact.
// Walking the linked list directly keeps saving O(n).
static bool writeToFile(Editor &ed, const std::string &path)
{
    std::ofstream out(path.c_str());
    if (!out.is_open())
        return false;

    typedef DoublyLinkedList<Line>::Node Node;
    for (Node *cur = ed.document.getHead(); cur != nullptr; cur = cur->next)
    {
        out << cur->data.text << '\n';
    }
    out.flush();
    if (!out.good())
        return false;

    // Check close too: some write failures are only reported when the stream
    // flushes its final buffer during close.
    out.close();
    return !out.fail();
}

bool newDocument(Editor &ed)
{
    // reset() clears the document, both stacks and the bookmarks, puts the
    // cursor at (0, 0), clears the filename and the modified flag, and leaves
    // one empty line to type into.
    ed.reset();
    return true;
}

bool openFile(Editor &ed, const std::string &path)
{
    if (path.empty())
    {
        std::cout << "Error: no file name given.\n";
        return false;
    }

    std::ifstream in(path.c_str());
    if (!in.is_open())
    {
        std::cout << "Error: cannot open '" << path << "'.\n";
        return false;
    }

    // Build the replacement separately so a failed read leaves the current
    // document, cursor, history, and metadata untouched.
    DoublyLinkedList<Line> temp;
    std::string lineText;
    while (std::getline(in, lineText))
    {
        if (!lineText.empty() && lineText[lineText.size() - 1] == '\r')
            lineText.erase(lineText.size() - 1);

        Line l;
        l.text = lineText;
        temp.insertAtTail(l);
    }
    if (in.bad())
    {
        std::cout << "Error: failed while reading '" << path << "'.\n";
        return false;
    }

    // An empty file becomes one editable empty line, matching a new document.
    if (temp.isEmpty())
    {
        Line blank;
        temp.insertAtTail(blank);
    }

    // Install the new content and discard state that refers to the old file.
    ed.document = temp;
    ed.undoStack.clear();
    ed.redoStack.clear();
    ed.bookmarks.clear();
    ed.cursorLine = 0;
    ed.cursorCol = 0;
    ed.setFilename(path);
    ed.setModified(false);
    return true;
}

bool saveFile(Editor &ed)
{
    if (ed.getFilename().empty())
    {
        std::cout << "Error: no file name yet, use Save As.\n";
        return false;
    }
    if (!writeToFile(ed, ed.getFilename()))
    {
        std::cout << "Error: cannot write '" << ed.getFilename() << "'.\n";
        return false;
    }
    ed.setModified(false);
    return true;
}

bool saveFileAs(Editor &ed, const std::string &path)
{
    if (path.empty())
    {
        std::cout << "Error: no file name given.\n";
        return false;
    }
    if (!writeToFile(ed, path))
    {
        std::cout << "Error: cannot write '" << path << "'.\n";
        return false;
    }
    ed.setFilename(path);
    ed.setModified(false);
    return true;
}

bool exitEditor(Editor &ed)
{
    return !ed.isModified();
}
